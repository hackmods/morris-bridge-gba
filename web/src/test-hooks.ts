import type { GbaEmulator } from './emulator';
import type { InputQueue } from './input-queue';

export interface CanvasSample {
  width: number;
  height: number;
  nonBlackPixels: number;
  totalPixels: number;
  averageLuminance: number;
}

export interface SaveProgressSnapshot {
  magic: number;
  version: number;
  maxUnlockedLevel: number;
  clearedLevels: number;
  unlockedEngines: number;
  packedStars: number;
  introSeenMask: number;
}

export interface PlaytestTestHooks {
  getFrameCount(): number;
  getStatusText(): string;
  isReady(): boolean;
  hasError(): boolean;
  enqueue(command: string): void;
  tapButton(button: string): void;
  holdButton(button: string, ms?: number): Promise<void>;
  holdButtons(buttons: string[], ms?: number): Promise<void>;
  loadWalkthroughSave(): Promise<void>;
  readWalkthroughSaveLevel(): number;
  readSaveProgress(): SaveProgressSnapshot | null;
  debugSaveState?(): Record<string, unknown>;
  sampleCanvas(): CanvasSample;
  waitForFrames(minFrames: number, timeoutMs?: number): Promise<number>;
}

declare global {
  interface Window {
    __TEST__?: PlaytestTestHooks;
  }
}

const SAVE_MAGIC = 0x47425254; // 'TRBG' little-endian

function decodeSaveProgress(save: Uint8Array | null): SaveProgressSnapshot | null {
  if (!save || save.length < 20) {
    return null;
  }

  const magic =
    save[0]! | (save[1]! << 8) | (save[2]! << 16) | (save[3]! << 24);

  if (magic !== SAVE_MAGIC) {
    return null;
  }

  return {
    magic,
    version: save[4]!,
    maxUnlockedLevel: save[5]!,
    clearedLevels: save[6]!,
    unlockedEngines: save[7]!,
    packedStars: save[8]!,
    introSeenMask: save[15]!,
  };
}

export function installTestHooks(
  emulator: GbaEmulator,
  inputQueue: InputQueue,
  statusBar: HTMLElement,
): void {
  if (import.meta.env.PROD) {
    return;
  }

  window.__TEST__ = {
    getFrameCount: () => emulator.getFrameCount(),
    getStatusText: () => statusBar.textContent ?? '',
    isReady: () => statusBar.classList.contains('ready'),
    hasError: () => {
      const banner = document.getElementById('error-banner');
      return Boolean(banner && !banner.classList.contains('hidden'));
    },
    enqueue: (command) => inputQueue.enqueueInput(command),
    tapButton: (button) => {
      emulator.pressButton(button);
      window.setTimeout(() => emulator.unpressButton(button), 100);
    },
    holdButton: (button, ms = 300) =>
      new Promise<void>((resolve) => {
        emulator.pressButton(button);
        window.setTimeout(() => {
          emulator.unpressButton(button);
          resolve();
        }, ms);
      }),
    holdButtons: (buttons, ms = 300) =>
      new Promise<void>((resolve) => {
        for (const button of buttons) {
          emulator.pressButton(button);
        }

        window.setTimeout(() => {
          for (const button of buttons) {
            emulator.unpressButton(button);
          }

          resolve();
        }, ms);
      }),
    loadWalkthroughSave: async () => {
      const response = await fetch('/saves/gordon_hill_walkthrough.sav');

      if (!response.ok) {
        throw new Error('Missing /saves/gordon_hill_walkthrough.sav — run tools/gen_playtest_save.py');
      }

      const bytes = new Uint8Array(await response.arrayBuffer());
      await emulator.injectSave(bytes);

      await new Promise<void>((resolve) => {
        window.setTimeout(resolve, 2_000);
      });
    },
    readWalkthroughSaveLevel: () => {
      const save = emulator.readSave();
      return save ? save[5]! : -1;
    },
    readSaveProgress: () => {
      const module = emulator.getModule();
      let save = emulator.readSave();

      if (module?.saveName) {
        try {
          const raw = module.FS.readFile(module.saveName) as Uint8Array;
          save = raw.subarray(0, Math.min(raw.length, 32768));
        } catch {
          // Fall back to getSave().
        }
      }

      return decodeSaveProgress(save);
    },
    debugSaveState: () => {
      const module = emulator.getModule();
      if (!module) {
        return { error: 'no module' };
      }

      const saveName = module.saveName ?? null;
      let fsBytes: number[] | null = null;
      let fsError: string | null = null;

      if (saveName) {
        try {
          const raw = module.FS.readFile(saveName) as Uint8Array;
          fsBytes = Array.from(raw.subarray(0, 20));
        } catch (error) {
          fsError = error instanceof Error ? error.message : String(error);
        }
      }

      const viaGetSave = emulator.readSave();

      return {
        saveName,
        listSaves: module.listSaves(),
        fsBytes,
        fsError,
        getSaveBytes: viaGetSave ? Array.from(viaGetSave.subarray(0, 20)) : null,
      };
    },
    sampleCanvas: () => sampleCanvas(document.getElementById('game-canvas') as HTMLCanvasElement),
    waitForFrames: (minFrames, timeoutMs = 30_000) =>
      waitForFrames(emulator, minFrames, timeoutMs),
  };
}

function sampleCanvas(canvas: HTMLCanvasElement): CanvasSample {
  const width = canvas.width;
  const height = canvas.height;
  const totalPixels = width * height;

  const gl = canvas.getContext('webgl2') ?? canvas.getContext('webgl');
  if (gl) {
    const pixels = new Uint8Array(width * height * 4);
    gl.readPixels(0, 0, width, height, gl.RGBA, gl.UNSIGNED_BYTE, pixels);

    let nonBlackPixels = 0;
    let luminanceSum = 0;

    for (let i = 0; i < pixels.length; i += 4) {
      const red = pixels[i] ?? 0;
      const green = pixels[i + 1] ?? 0;
      const blue = pixels[i + 2] ?? 0;
      const alpha = pixels[i + 3] ?? 0;
      const luminance = 0.299 * red + 0.587 * green + 0.114 * blue;

      luminanceSum += luminance;

      if (alpha > 0 && (red > 10 || green > 10 || blue > 10)) {
        nonBlackPixels += 1;
      }
    }

    return {
      width,
      height,
      nonBlackPixels,
      totalPixels,
      averageLuminance: luminanceSum / totalPixels,
    };
  }

  const context = canvas.getContext('2d');
  if (context) {
    const image = context.getImageData(0, 0, width, height);
    let nonBlackPixels = 0;
    let luminanceSum = 0;

    for (let i = 0; i < image.data.length; i += 4) {
      const red = image.data[i] ?? 0;
      const green = image.data[i + 1] ?? 0;
      const blue = image.data[i + 2] ?? 0;
      const alpha = image.data[i + 3] ?? 0;
      const luminance = 0.299 * red + 0.587 * green + 0.114 * blue;

      luminanceSum += luminance;

      if (alpha > 0 && (red > 10 || green > 10 || blue > 10)) {
        nonBlackPixels += 1;
      }
    }

    return {
      width,
      height,
      nonBlackPixels,
      totalPixels,
      averageLuminance: luminanceSum / totalPixels,
    };
  }

  return {
    width,
    height,
    nonBlackPixels: 0,
    totalPixels,
    averageLuminance: 0,
  };
}

async function waitForFrames(
  emulator: GbaEmulator,
  minFrames: number,
  timeoutMs: number,
): Promise<number> {
  const deadline = Date.now() + timeoutMs;

  while (Date.now() < deadline) {
    const count = emulator.getFrameCount();

    if (count >= minFrames) {
      return count;
    }

    await new Promise<void>((resolve) => {
      requestAnimationFrame(() => resolve());
    });
  }

  throw new Error(
    `Timed out waiting for ${minFrames} frames (got ${emulator.getFrameCount()})`,
  );
}
