import { describe, expect, it } from 'vitest';

import { engineById } from './engines';
import { loadLevel } from './level-io';
import { LEVELS } from './levels';
import { spanHeat, spanIsWeak } from './materials';
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

  it('loads Gordon harder than Percy on the same two-span deck', () => {
    const level = LEVELS[0];
    const percyWorld = new World();
    const gordonWorld = new World();
    loadLevel(percyWorld, level);
    loadLevel(gordonWorld, level);
    const start = percyWorld.node(percyWorld.startId);
    const end = percyWorld.node(percyWorld.endId);
    expect(start && end).toBeTruthy();
    if (!start || !end) {
      return;
    }
    const midX = (start.x + end.x) / 2;
    const midPercy = percyWorld.addNode(midX, start.y);
    const midGordon = gordonWorld.addNode(midX, start.y);
    expect(percyWorld.addBeam(start.id, midPercy.id, 'road')).toBeTruthy();
    expect(percyWorld.addBeam(midPercy.id, end.id, 'road')).toBeTruthy();
    expect(gordonWorld.addBeam(start.id, midGordon.id, 'road')).toBeTruthy();
    expect(gordonWorld.addBeam(midGordon.id, end.id, 'road')).toBeTruthy();
    const percy = engineById('percy');
    const gordon = engineById('gordon');
    percyWorld.beginTest(percy.weight, percy.speed, percy.coaches);
    gordonWorld.beginTest(gordon.weight, gordon.speed, gordon.coaches);
    for (let i = 0; i < 90; i += 1) {
      percyWorld.step(1 / 60);
      gordonWorld.step(1 / 60);
    }
    const peak = (world: World) =>
      Math.max(0, ...[...world.beams.values()].map((beam) => beam.stress));
    expect(peak(gordonWorld)).toBeGreaterThan(peak(percyWorld));
  });

  it('carries Gordon across the Express prestige run', () => {
    const level = LEVELS[7];
    const world = new World();
    expect(buildReference(world, level)).toBe(true);
    const gordon = engineById('gordon');
    expect(runCrossing(world, gordon.weight, gordon.speed, gordon.coaches, 40)).toBe('success');
  });
});

describe('build-mode stress preview', () => {
  it('marks a near-max road as weak before the test run', () => {
    const world = new World();
    loadLevel(world, LEVELS[0]);
    const start = world.node(world.startId);
    expect(start).toBeTruthy();
    if (!start) {
      return;
    }
    const far = world.addNode(start.x + 210, start.y);
    const preview = world.previewBeam(start.id, far.id, 'road');
    expect(preview.ok).toBe(true);
    expect(preview.weak).toBe(true);
    expect(spanIsWeak(preview.length, 'road', 4)).toBe(true);
  });

  it('heats a long road more than a short timber brace', () => {
    expect(spanHeat(250, 'road', 4)).toBeGreaterThan(spanHeat(80, 'wood', 4));
    expect(spanHeat(120, 'road', 12.7)).toBeGreaterThan(spanHeat(120, 'road', 3));
  });
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
