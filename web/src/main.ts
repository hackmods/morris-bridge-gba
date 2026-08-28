import { GbaEmulator } from './emulator';
import { InputQueue } from './input-queue';
import { setupKeyboard, setEmulatorInputEnabled } from './keyboard';

function $(id: string): HTMLElement {
  const element = document.getElementById(id);
  if (!element) {
    throw new Error(`Missing element #${id}`);
  }
  return element;
}

function setEnabled(enabled: boolean): void {
  for (const id of [
    'btn-reset',
    'command-input',
    'btn-enqueue',
    'btn-pause-queue',
    'btn-clear-queue',
  ]) {
    const element = $(id) as HTMLButtonElement | HTMLInputElement;
    element.disabled = !enabled;
  }
}

function showError(message: string): void {
  const banner = $('error-banner');
  banner.textContent = message;
  banner.classList.remove('hidden');
}

function appendFeedLine(line: string): void {
  const feed = $('input-feed');
  const item = document.createElement('li');
  item.innerHTML = line.replace(
    /▶\s+(\w+)/,
    '▶ <span class="cmd">$1</span>',
  );
  feed.prepend(item);

  while (feed.children.length > 100) {
    feed.lastElementChild?.remove();
  }
}

async function main(): Promise<void> {
  const canvas = $('game-canvas') as HTMLCanvasElement;
  const statusBar = $('status-bar');
  const dropOverlay = $('drop-overlay');
  const canvasWrap = $('canvas-wrap');

  const emulator = new GbaEmulator(canvas);
  const inputQueue = new InputQueue(emulator);

  emulator.onStatus((message, status) => {
    statusBar.textContent = message;
    statusBar.classList.toggle('ready', status === 'ready');
  });

  inputQueue.onFeed(appendFeedLine);

  const holdSlider = $('hold-ms') as HTMLInputElement;
  const holdLabel = $('hold-ms-label');

  holdSlider.addEventListener('input', () => {
    const value = Number(holdSlider.value);
    holdLabel.textContent = `${value} ms`;
    inputQueue.setHoldMs(value);
  });

  const commandInput = $('command-input') as HTMLInputElement;
  commandInput.addEventListener('focus', () => {
    setEmulatorInputEnabled(emulator, false);
  });
  commandInput.addEventListener('blur', () => {
    setEmulatorInputEnabled(emulator, true);
  });

  $('command-form').addEventListener('submit', (event) => {
    event.preventDefault();
    const input = $('command-input') as HTMLInputElement;
    const value = input.value.trim();

    if (!value) {
      return;
    }

    try {
      inputQueue.enqueueInput(value);
      input.value = '';
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      appendFeedLine(`Error: ${message}`);
    }
  });

  $('btn-pause-queue').addEventListener('click', () => {
    const paused = inputQueue.togglePaused();
    ($('btn-pause-queue') as HTMLButtonElement).textContent = paused
      ? 'Resume queue'
      : 'Pause queue';
  });

  $('btn-clear-queue').addEventListener('click', () => {
    inputQueue.clear();
  });

  $('btn-reset').addEventListener('click', () => {
    emulator.softReset();
    appendFeedLine('Soft reset');
  });

  $('rom-upload').addEventListener('change', async (event) => {
    const input = event.target as HTMLInputElement;
    const file = input.files?.[0];

    if (!file) {
      return;
    }

    try {
      await emulator.loadRomFile(file);
      appendFeedLine(`Loaded ROM: ${file.name}`);
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      showError(message);
      appendFeedLine(`ROM error: ${message}`);
    } finally {
      input.value = '';
    }
  });

  const preventDefaults = (event: DragEvent): void => {
    event.preventDefault();
    event.stopPropagation();
  };

  for (const eventName of ['dragenter', 'dragover', 'dragleave', 'drop'] as const) {
    canvasWrap.addEventListener(eventName, preventDefaults);
  }

  canvasWrap.addEventListener('dragenter', () => {
    dropOverlay.classList.remove('hidden');
    dropOverlay.classList.add('visible');
  });

  canvasWrap.addEventListener('dragleave', (event) => {
    if (event.currentTarget === event.target) {
      dropOverlay.classList.add('hidden');
      dropOverlay.classList.remove('visible');
    }
  });

  canvasWrap.addEventListener('drop', async (event) => {
    dropOverlay.classList.add('hidden');
    dropOverlay.classList.remove('visible');

    const file = event.dataTransfer?.files[0];
    if (!file || !file.name.toLowerCase().endsWith('.gba')) {
      appendFeedLine('Drop ignored — need a .gba file');
      return;
    }

    try {
      await emulator.loadRomFile(file);
      appendFeedLine(`Dropped ROM: ${file.name}`);
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      showError(message);
      appendFeedLine(`ROM error: ${message}`);
    }
  });

  try {
    await emulator.init();
    setupKeyboard(emulator);
    setEnabled(true);
    appendFeedLine('Morris Bridge ready — keyboard or queue inputs accepted');
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    showError(message);
    statusBar.textContent = 'Failed to start emulator';
  }
}

void main();
