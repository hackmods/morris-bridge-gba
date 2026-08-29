import { GbaEmulator } from './emulator';
import { InputQueue } from './input-queue';
import { bindResponsiveCanvas } from './layout';
import { setupKeyboard, setEmulatorInputEnabled } from './keyboard';
import { installTestHooks } from './test-hooks';

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

const LORE_READY_LINES = [
  'Thomas\'s Bridge ready — keyboard or queue inputs accepted',
  'Thomas\'s Bridge ready — no confusion, no delay',
  'Thomas\'s Bridge ready — Percy says listen to the rails',
  'Thomas\'s Bridge ready — really useful builders welcome',
  'Thomas\'s Bridge ready — timetable looks normal today',
  'Thomas\'s Bridge ready — watch for rare PEEP PEEP banners',
];

function pickReadyFeedLine(): string {
  if (Math.floor(Math.random() * 100) >= 8) {
    return LORE_READY_LINES[0];
  }

  const index = 1 + Math.floor(Math.random() * (LORE_READY_LINES.length - 1));
  return LORE_READY_LINES[index];
}

async function main(): Promise<void> {
  const canvas = $('game-canvas') as HTMLCanvasElement;
  const statusBar = $('status-bar');
  const dropOverlay = $('drop-overlay');
  const canvasWrap = $('canvas-wrap');

  const emulator = new GbaEmulator(canvas);
  const inputQueue = new InputQueue(emulator);
  const unbindCanvasLayout = bindResponsiveCanvas(canvas, canvasWrap);

  $('btn-theater').addEventListener('click', () => {
    document.body.classList.toggle('theater-mode');
    const enabled = document.body.classList.contains('theater-mode');
    ($('btn-theater') as HTMLButtonElement).textContent = enabled
      ? 'Exit theater'
      : 'Theater';
  });

  $('btn-fullscreen').addEventListener('click', async () => {
    if (!document.fullscreenElement) {
      await canvasWrap.requestFullscreen();
      return;
    }

    await document.exitFullscreen();
  });

  document.addEventListener('fullscreenchange', () => {
    const enabled = Boolean(document.fullscreenElement);
    ($('btn-fullscreen') as HTMLButtonElement).textContent = enabled
      ? 'Exit fullscreen'
      : 'Fullscreen';
  });

  window.addEventListener('beforeunload', () => {
    unbindCanvasLayout();
  });

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
    const walkthrough = new URLSearchParams(window.location.search).get('walkthrough');
    if (walkthrough === 'gordon') {
      await emulator.prepareWalkthroughSave('/saves/gordon_hill_walkthrough.sav');
    } else if (walkthrough === 'vicarstown') {
      await emulator.prepareWalkthroughSave('/saves/all_unlocked.sav');
    }

    await emulator.init();
    setupKeyboard(emulator);
    installTestHooks(emulator, inputQueue, statusBar);
    setEnabled(true);
    appendFeedLine(pickReadyFeedLine());
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    showError(message);
    statusBar.textContent = 'Failed to start emulator';
  }
}

void main();
