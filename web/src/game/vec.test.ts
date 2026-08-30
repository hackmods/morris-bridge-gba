import { describe, expect, it } from 'vitest';

import { dist, lerp } from './vec';

describe('vec', () => {
  it('measures distance and interpolates', () => {
    expect(dist({ x: 0, y: 0 }, { x: 3, y: 4 })).toBe(5);
    expect(lerp(0, 10, 0.5)).toBe(5);
  });
});
