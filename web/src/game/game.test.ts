import { describe, expect, it } from 'vitest';

import { engineById } from './engines';
import { loadLevel } from './level-io';
import { LEVELS } from './levels';
import { World, runCrossing } from './physics';
import { starsForCost } from './score';
import { buildAllReferences, buildReference } from './solutions';

describe('campaign content', () => {
  it('ships eight crossings with a full material roster by the docks', () => {
    expect(LEVELS).toHaveLength(8);
    expect(LEVELS[0].tools).toEqual(['road', 'wood']);
    expect(LEVELS[4].tools).toContain('steel');
    expect(LEVELS[7].id).toBe('express');
  });
});

describe('reference bridges', () => {
  it('builds a road path on every official layout', () => {
    for (const row of buildAllReferences()) {
      expect(row.ok, row.id).toBe(true);
      expect(row.path, row.id).toBe(true);
      expect(row.cost).toBeGreaterThan(200);
      const level = LEVELS.find((item) => item.id === row.id);
      expect(row.cost).toBeLessThanOrEqual(level?.budget ?? 0);
    }
  });

  it.each(LEVELS.map((level) => [level.id, level] as const))(
    'carries Thomas across %s',
    (id, level) => {
      const world = new World();
      expect(buildReference(world, level), id).toBe(true);
      const thomas = engineById('thomas');
      const status = runCrossing(world, thomas.weight, thomas.speed, thomas.coaches, 28);
      expect(status, id).toBe('success');
    },
  );
});

describe('welding rules', () => {
  it('rejects a duplicate weld and a zero-length span', () => {
    const world = new World();
    loadLevel(world, LEVELS[0]);
    const start = world.node(world.startId);
    const end = world.node(world.endId);
    expect(start && end).toBeTruthy();
    if (!start || !end) {
      return;
    }
    const mid = world.addNode((start.x + end.x) / 2, start.y);
    expect(world.addBeam(start.id, mid.id, 'road')).toBeTruthy();
    expect(world.addBeam(start.id, mid.id, 'road')).toBeNull();
    expect(world.previewBeam(start.id, start.id, 'road').ok).toBe(false);
  });

  it('blocks the test run until a road path exists', () => {
    const world = new World();
    loadLevel(world, LEVELS[0]);
    expect(world.hasRoadPath()).toBe(false);
    expect(world.beginTest(4, 1, 0)).toBe('no_path');
  });

  it('snaps an overlong single span instead of cheating the creek', () => {
    const world = new World();
    loadLevel(world, LEVELS[0]);
    const start = world.startId;
    const end = world.endId;
    const preview = world.previewBeam(start, end, 'road');
    expect(preview.ok).toBe(false);
  });
});

describe('scoring', () => {
  it('awards three stars at or under par', () => {
    expect(starsForCost(900, 920)).toBe(3);
    expect(starsForCost(1200, 920)).toBe(2);
    expect(starsForCost(2000, 920)).toBe(1);
  });
});
