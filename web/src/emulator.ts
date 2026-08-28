import mGBA, { type mGBAEmulator } from '@thenick775/mgba-wasm';

export type EmulatorStatus = 'initializing' | 'ready' | 'error';

export type StatusListener = (message: string, status: EmulatorStatus) => void;

const DEFAULT_ROM_URL = '/rom/thomas_bridge.gba';
const DEFAULT_ROM_NAME = 'thomas_bridge.gba';

export class GbaEmulator {
  private module: mGBAEmulator | null = null;
  private loadedRomPath: string | null = null;
  private loadedRomName = DEFAULT_ROM_NAME;
  private statusListener: StatusListener | null = null;
  private frameCount = 0;

  constructor(private readonly canvas: HTMLCanvasElement) {}

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

      module.setCoreSettings({
        showFpsCounter: false,
        videoSync: true,
        audioSync: true,
      });

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

    await new Promise<void>((resolve, reject) => {
      module.uploadRom(file, () => {
        try {
          const romPath = `${module.filePaths().gamePath}/${file.name}`;
          const loaded = module.loadGame(romPath);

          if (!loaded) {
            reject(new Error(`Failed to load ${file.name}`));
            return;
          }

          this.loadedRomPath = romPath;
          this.loadedRomName = file.name;
          module.resumeGame();
          this.setStatus(`Running ${file.name}`, 'ready');
          resolve();
        } catch (error) {
          reject(error);
        }
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
