import { describe, expect, it } from 'vitest';

import { computeCanvasScale, GBA_HEIGHT, GBA_WIDTH } from './layout';

describe('computeCanvasScale', () => {
  it('preserves the GBA aspect ratio when fitting a container', () => {
    expect(computeCanvasScale(720, 480)).toBe(3);
    expect(computeCanvasScale(480, 320)).toBe(2);
  });

  it('caps scale at the configured maximum', () => {
    expect(computeCanvasScale(2000, 2000, 4)).toBe(4);
  });

  it('falls back to a minimum scale for empty containers', () => {
    expect(computeCanvasScale(0, 0)).toBe(3);
  });

  it('exports native GBA dimensions', () => {
    expect(GBA_WIDTH).toBe(240);
    expect(GBA_HEIGHT).toBe(160);
  });
});
