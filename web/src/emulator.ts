import mGBA, { type mGBAEmulator } from '@thenick775/mgba-wasm';

export type EmulatorStatus = 'initializing' | 'ready' | 'error';

export type StatusListener = (message: string, status: EmulatorStatus) => void;

const DEFAULT_ROM_URL = '/rom/thomas_bridge.gba';
const DEFAULT_ROM_NAME = 'thomas_bridge.gba';
const SRAM_SIZE = 32768;

export class GbaEmulator {
  private module: mGBAEmulator | null = null;
  private loadedRomPath: string | null = null;
  private loadedRomName = DEFAULT_ROM_NAME;
  private statusListener: StatusListener | null = null;
  private frameCount = 0;
  private pendingSaveSeed: Uint8Array | null = null;

  constructor(private readonly canvas: HTMLCanvasElement) {}

  /** Fetch SRAM bytes to seed before the next ROM load (used by walkthrough E2E). */
  async prepareWalkthroughSave(url: string): Promise<void> {
    const response = await fetch(url);

    if (!response.ok) {
      throw new Error(`Missing walkthrough save at ${url}`);
    }

    this.pendingSaveSeed = new Uint8Array(await response.arrayBuffer());
  }

  onStatus(listener: StatusListener): void {
    this.statusListener = listener;
  }

  getModule(): mGBAEmulator | null {
    return this.module;
  }

  getLoadedRomName(): string {
    return this.loadedRomName;
  }

  async init(): Promise<void> {
    this.assertCrossOriginIsolation();
    this.setStatus('Starting mGBA…', 'initializing');

    try {
      const module = await mGBA({ canvas: this.canvas });
      await module.FSInit();

      module.addCoreCallbacks({
        videoFrameEndedCallback: () => {
          this.frameCount += 1;
        },
      });

      if (this.pendingSaveSeed) {
        module.setCoreSettings({
          autoSaveStateEnable: false,
          restoreAutoSaveStateOnLoad: false,
        });
      }

      this.module = module;
      const version = `${module.version.projectName} ${module.version.projectVersion}`;
      this.setStatus(`mGBA ${version} — loading ROM…`, 'initializing');

      await this.loadRomFromUrl(DEFAULT_ROM_URL, DEFAULT_ROM_NAME);
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      this.setStatus(`Emulator init failed: ${message}`, 'error');
      throw error;
    }
  }

  async loadRomFromUrl(url: string, fileName: string): Promise<void> {
    const response = await fetch(url);

    if (!response.ok) {
      throw new Error(
        `ROM not found at ${url}. Run ./scripts/sync_rom_to_web.sh from the repo root.`,
      );
    }

    const blob = await response.blob();
    const file = new File([blob], fileName, { type: 'application/octet-stream' });
    await this.loadRomFile(file);
  }

  async loadRomFile(file: File): Promise<void> {
    const module = this.requireModule();
    this.setStatus(`Uploading ${file.name}…`, 'initializing');

    const saveSeed = this.pendingSaveSeed;
    this.pendingSaveSeed = null;

    await this.uploadRom(module, file);

    const romPath = `${module.filePaths().gamePath}/${file.name}`;

    if (saveSeed) {
      this.clearAutoSaveState(module, file.name);
      await this.uploadSave(module, file.name, saveSeed);
    }

    if (!module.loadGame(romPath)) {
      throw new Error(`Failed to load ${file.name}`);
    }

    this.loadedRomPath = romPath;
    this.loadedRomName = file.name;
    module.resumeGame();
    this.setStatus(`Running ${file.name}`, 'ready');
  }

  private uploadRom(module: mGBAEmulator, file: File): Promise<void> {
    return new Promise((resolve) => {
      module.uploadRom(file, () => resolve());
    });
  }

  private uploadSave(module: mGBAEmulator, romFileName: string, saveSeed: Uint8Array): Promise<void> {
    const saveFileName = romFileName.replace(/\.gba$/i, '.sav');
    const saveFile = new File([this.padSave(saveSeed)], saveFileName, {
      type: 'application/octet-stream',
    });

    return new Promise((resolve, reject) => {
      module.uploadSaveOrSaveState(saveFile, () => {
        void module.FSSync().then(resolve).catch(reject);
      });
    });
  }

  softReset(): void {
    const module = this.requireModule();

    if (this.loadedRomPath) {
      module.loadGame(this.loadedRomPath);
      module.resumeGame();
      this.setStatus(`Reset ${this.loadedRomName}`, 'ready');
      return;
    }

    module.quickReload();
    module.resumeGame();
    this.setStatus('Soft reset', 'ready');
  }

  pressButton(name: string): void {
    this.requireModule().buttonPress(name);
  }

  unpressButton(name: string): void {
    this.requireModule().buttonUnpress(name);
  }

  getFrameCount(): number {
    return this.frameCount;
  }

  /** Write GBA SRAM save bytes and reload so the game picks up progress. */
  async injectSave(saveBytes: Uint8Array): Promise<void> {
    const module = this.requireModule();

    if (!this.loadedRomPath) {
      throw new Error('Load a ROM before injecting save data');
    }

    await this.uploadSave(module, this.loadedRomName, saveBytes);

    // Remount the ROM so mGBA reloads the uploaded .sav into emulated SRAM.
    module.quitGame();

    if (!module.loadGame(this.loadedRomPath)) {
      throw new Error('Failed to reload game with injected save');
    }

    module.resumeGame();
    this.setStatus(`Loaded save ${this.loadedRomName}`, 'ready');
  }

  /** Read back emulated SRAM (32 KiB) for test verification. */
  readSave(): Uint8Array | null {
    return this.requireModule().getSave();
  }

  private padSave(saveBytes: Uint8Array): Uint8Array {
    const padded = new Uint8Array(SRAM_SIZE);
    padded.set(saveBytes.subarray(0, Math.min(saveBytes.length, padded.length)));
    return padded;
  }

  private clearAutoSaveState(module: mGBAEmulator, romFileName: string): void {
    const autoSavePath = `${module.filePaths().autosave}/${romFileName.replace(/\.gba$/i, '')}_auto.ss`;

    try {
      module.FS.unlink(autoSavePath);
    } catch {
      // No prior auto save state for this ROM.
    }
  }

  private requireModule(): mGBAEmulator {
    if (!this.module) {
      throw new Error('Emulator not initialized');
    }

    return this.module;
  }

  private setStatus(message: string, status: EmulatorStatus): void {
    this.statusListener?.(message, status);
  }

  private assertCrossOriginIsolation(): void {
    if (typeof crossOriginIsolated !== 'undefined' && crossOriginIsolated) {
      return;
    }

    throw new Error(
      'Cross-origin isolation is required for mGBA threads.\n' +
        'Run `npm run dev` and open the served URL — do not open index.html as file://',
    );
  }
}
