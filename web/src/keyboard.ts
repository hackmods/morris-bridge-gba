import type { GbaEmulator } from './emulator';

const KEY_BINDINGS: Array<[string, string]> = [
  ['ArrowUp', 'Up'],
  ['ArrowDown', 'Down'],
  ['ArrowLeft', 'Left'],
  ['ArrowRight', 'Right'],
  ['z', 'A'],
  ['Z', 'A'],
  ['x', 'B'],
  ['X', 'B'],
  ['Enter', 'Start'],
  ['Backspace', 'Select'],
  ['r', 'R'],
  ['R', 'R'],
];

const PREVENT_DEFAULT_KEYS = new Set([
  'ArrowUp',
  'ArrowDown',
  'ArrowLeft',
  'ArrowRight',
  ' ',
]);

function isTypingTarget(target: EventTarget | null): boolean {
  if (!(target instanceof HTMLElement)) {
    return false;
  }

  return (
    target instanceof HTMLInputElement ||
    target instanceof HTMLTextAreaElement ||
    target.isContentEditable
  );
}

export function setupKeyboard(emulator: GbaEmulator): void {
  const module = emulator.getModule();

  if (!module) {
    return;
  }

  for (const [key, button] of KEY_BINDINGS) {
    module.bindKey(key, button);
  }

  window.addEventListener(
    'keydown',
    (event) => {
      if (isTypingTarget(event.target)) {
        return;
      }

      if (PREVENT_DEFAULT_KEYS.has(event.key)) {
        event.preventDefault();
      }
    },
    { passive: false },
  );
}

export function setEmulatorInputEnabled(emulator: GbaEmulator, enabled: boolean): void {
  emulator.getModule()?.toggleInput(enabled);
}
