import { describe, expect, it, vi } from 'vitest';

import type { GbaEmulator } from './emulator';
import { InputQueue, parseCommandTokens } from './input-queue';

describe('parseCommandTokens', () => {
  it('parses valid tokens in order', () => {
    expect(parseCommandTokens('right a start')).toEqual(['Right', 'A', 'Start']);
  });

  it('ignores case and extra whitespace', () => {
    expect(parseCommandTokens('  Up   DOWN  ')).toEqual(['Up', 'Down']);
  });

  it('returns an empty list for blank input', () => {
    expect(parseCommandTokens('   ')).toEqual([]);
  });

  it('throws on unknown tokens', () => {
    expect(() => parseCommandTokens('jump')).toThrow(/Unknown command "jump"/);
  });
});

describe('InputQueue', () => {
  it('enqueues parsed commands while paused', () => {
    const emulator = {
      pressButton: vi.fn(),
      unpressButton: vi.fn(),
    } as unknown as GbaEmulator;

    const queue = new InputQueue(emulator);
    queue.togglePaused();
    queue.enqueueInput('a b');

    expect(queue.getPendingCount()).toBe(2);
  });

  it('clears pending commands', () => {
    const emulator = {
      pressButton: vi.fn(),
      unpressButton: vi.fn(),
    } as unknown as GbaEmulator;

    const queue = new InputQueue(emulator);
    queue.togglePaused();
    queue.enqueueInput('start select');
    queue.clear();

    expect(queue.getPendingCount()).toBe(0);
  });

  it('tracks pause state', () => {
    const emulator = {
      pressButton: vi.fn(),
      unpressButton: vi.fn(),
    } as unknown as GbaEmulator;

    const queue = new InputQueue(emulator);
    expect(queue.isPaused()).toBe(false);
    expect(queue.togglePaused()).toBe(true);
    expect(queue.isPaused()).toBe(true);
    expect(queue.togglePaused()).toBe(false);
  });
});
