import type { GbaEmulator } from './emulator';

export const GBA_BUTTONS = [
  'Up',
  'Down',
  'Left',
  'Right',
  'A',
  'B',
  'Start',
  'Select',
  'R',
] as const;

export type GbaButton = (typeof GBA_BUTTONS)[number];

const TOKEN_TO_BUTTON: Record<string, GbaButton> = {
  up: 'Up',
  down: 'Down',
  left: 'Left',
  right: 'Right',
  a: 'A',
  b: 'B',
  start: 'Start',
  select: 'Select',
  r: 'R',
};

export function parseCommandTokens(input: string): GbaButton[] {
  const tokens = input.trim().toLowerCase().split(/\s+/).filter(Boolean);
  const buttons: GbaButton[] = [];

  for (const token of tokens) {
    const button = TOKEN_TO_BUTTON[token];

    if (!button) {
      throw new Error(`Unknown command "${token}". Use: ${Object.keys(TOKEN_TO_BUTTON).join(', ')}`);
    }

    buttons.push(button);
  }

  return buttons;
}

export type FeedListener = (line: string) => void;

export class InputQueue {
  private readonly queue: GbaButton[] = [];
  private paused = false;
  private processing = false;
  private holdMs = 100;
  private feedListener: FeedListener | null = null;

  constructor(private readonly emulator: GbaEmulator) {}

  onFeed(listener: FeedListener): void {
    this.feedListener = listener;
  }

  setHoldMs(ms: number): void {
    this.holdMs = ms;
  }

  enqueueTokens(buttons: GbaButton[]): void {
    this.queue.push(...buttons);
    this.log(`Queued ${buttons.join(', ')} (${this.queue.length} pending)`);
    void this.pump();
  }

  enqueueInput(input: string): void {
    const buttons = parseCommandTokens(input);

    if (buttons.length === 0) {
      return;
    }

    this.enqueueTokens(buttons);
  }

  togglePaused(): boolean {
    this.paused = !this.paused;
    this.log(this.paused ? 'Queue paused' : 'Queue resumed');

    if (!this.paused) {
      void this.pump();
    }

    return this.paused;
  }

  clear(): void {
    this.queue.length = 0;
    this.log('Queue cleared');
  }

  getPendingCount(): number {
    return this.queue.length;
  }

  isPaused(): boolean {
    return this.paused;
  }

  private async pump(): Promise<void> {
    if (this.processing || this.paused) {
      return;
    }

    this.processing = true;

    while (this.queue.length > 0 && !this.paused) {
      const button = this.queue.shift();
      if (!button) {
        break;
      }

      await this.playButton(button);
      await this.waitForNextFrame();
    }

    this.processing = false;

    if (this.queue.length > 0 && !this.paused) {
      void this.pump();
    }
  }

  private async playButton(button: GbaButton): Promise<void> {
    const timestamp = new Date().toLocaleTimeString();
    this.log(`[${timestamp}] ▶ ${button}`);
    this.emulator.pressButton(button);
    await this.sleep(this.holdMs);
    this.emulator.unpressButton(button);
  }

  private waitForNextFrame(): Promise<void> {
    return new Promise((resolve) => {
      requestAnimationFrame(() => resolve());
    });
  }

  private sleep(ms: number): Promise<void> {
    return new Promise((resolve) => {
      window.setTimeout(resolve, ms);
    });
  }

  private log(line: string): void {
    this.feedListener?.(line);
  }
}
