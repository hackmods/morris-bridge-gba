import { LEVELS, type LevelDef } from './levels';
import { buildSpans, deckRoad, loadLevel, type SpanSpec } from './level-io';
import { World } from './physics';

function knapford(level: LevelDef): SpanSpec[] {
  const y = level.start.y;
  const xs = [level.start.x, 440, 620, level.end.x];
  return [
    ...deckRoad(xs, y),
    { ax: 236, ay: 455, bx: 440, by: y, material: 'wood' },
    { ax: 824, ay: 455, bx: 620, by: y, material: 'wood' },
    { ax: 440, ay: y, bx: 620, by: y + 70, material: 'wood' },
    { ax: 620, ay: y, bx: 440, by: y + 70, material: 'wood' },
  ];
}

function tidmouth(level: LevelDef): SpanSpec[] {
  const y = level.start.y;
  const xs = [level.start.x, 420, 600, 780, level.end.x];
  return [
    ...deckRoad(xs, y),
    { ax: 220, ay: 458, bx: 420, by: y, material: 'wood' },
    { ax: 420, ay: y, bx: 600, by: y + 72, material: 'wood' },
    { ax: 780, ay: y, bx: 600, by: y + 72, material: 'wood' },
    { ax: 980, ay: 458, bx: 780, by: y, material: 'wood' },
    { ax: 420, ay: y + 72, bx: 600, by: y, material: 'wood' },
    { ax: 780, ay: y + 72, bx: 600, by: y, material: 'wood' },
  ];
}

function hill(level: LevelDef): SpanSpec[] {
  const y = level.start.y;
  const xs = [level.start.x, 430, 625, 820, level.end.x];
  return [
    ...deckRoad(xs, y),
    { ax: 226, ay: 430, bx: 430, by: y, material: 'wood' },
    { ax: 226, ay: 430, bx: 430, by: y + 86, material: 'wood' },
    { ax: 1024, ay: 430, bx: 820, by: y, material: 'wood' },
    { ax: 1024, ay: 430, bx: 820, by: y + 86, material: 'wood' },
    { ax: 430, ay: y, bx: 625, by: y + 86, material: 'wood' },
    { ax: 820, ay: y, bx: 625, by: y + 86, material: 'wood' },
    { ax: 430, ay: y + 86, bx: 625, by: y, material: 'wood' },
    { ax: 820, ay: y + 86, bx: 625, by: y, material: 'wood' },
  ];
}

function wellsworth(level: LevelDef): SpanSpec[] {
  const y = level.start.y;
  const xs = [level.start.x, 400, 575, 750, 925, level.end.x];
  return [
    ...deckRoad(xs, y),
    { ax: 200, ay: 478, bx: 400, by: y, material: 'wood' },
    { ax: 1090, ay: 478, bx: 925, by: y, material: 'wood' },
    { ax: 400, ay: y, bx: 575, by: y + 88, material: 'wood' },
    { ax: 750, ay: y, bx: 575, by: y + 88, material: 'wood' },
    { ax: 575, ay: y, bx: 400, by: y + 88, material: 'wood' },
    { ax: 750, ay: y, bx: 925, by: y + 88, material: 'wood' },
    { ax: 925, ay: y, bx: 750, by: y + 88, material: 'wood' },
  ];
}

function brendam(level: LevelDef): SpanSpec[] {
  const y = level.start.y;
  const xs = [level.start.x, 400, 580, 760, 940, level.end.x];
  return [
    ...deckRoad(xs, y),
    { ax: 196, ay: 492, bx: 400, by: y, material: 'steel' },
    { ax: 1164, ay: 492, bx: 940, by: y, material: 'steel' },
    { ax: 400, ay: y, bx: 580, by: y + 92, material: 'steel' },
    { ax: 760, ay: y, bx: 580, by: y + 92, material: 'steel' },
    { ax: 580, ay: y, bx: 760, by: y + 92, material: 'wood' },
    { ax: 940, ay: y, bx: 760, by: y + 92, material: 'wood' },
  ];
}

function vicarstown(level: LevelDef): SpanSpec[] {
  const y = level.start.y;
  const xs = [level.start.x, 400, 580, 760, 940, 1120, level.end.x];
  return [
    ...deckRoad(xs, y),
    { ax: 188, ay: 510, bx: 400, by: y, material: 'steel' },
    { ax: 1262, ay: 510, bx: 1120, by: y, material: 'steel' },
    { ax: 400, ay: y, bx: 580, by: y + 96, material: 'steel' },
    { ax: 760, ay: y, bx: 580, by: y + 96, material: 'steel' },
    { ax: 760, ay: y, bx: 940, by: y + 96, material: 'steel' },
    { ax: 1120, ay: y, bx: 940, by: y + 96, material: 'steel' },
    { ax: 580, ay: y, bx: 760, by: y + 96, material: 'wood' },
    { ax: 940, ay: y, bx: 1120, by: y + 96, material: 'wood' },
  ];
}

function ffarquhar(level: LevelDef): SpanSpec[] {
  const y = level.start.y;
  const islandY = 390;
  return [
    ...deckRoad([level.start.x, 400, 560], y),
    { ax: 560, ay: y, bx: 760, by: islandY, material: 'road' },
    { ax: 760, ay: islandY, bx: 820, by: islandY, material: 'road' },
    { ax: 820, ay: islandY, bx: 1000, by: y, material: 'road' },
    ...deckRoad([1000, 1180, level.end.x], y),
    { ax: 200, ay: 500, bx: 400, by: y, material: 'wood' },
    { ax: 400, ay: y, bx: 560, by: y + 70, material: 'steel' },
    { ax: 560, ay: y, bx: 760, by: islandY + 70, material: 'wood' },
    { ax: 820, ay: islandY, bx: 1000, by: y + 70, material: 'wood' },
    { ax: 1000, ay: y, bx: 1180, by: y + 70, material: 'steel' },
    { ax: 1372, ay: 500, bx: 1180, by: y, material: 'wood' },
  ];
}

function express(level: LevelDef): SpanSpec[] {
  const y = level.start.y;
  const xs = [level.start.x, 380, 540, 700, 860, 1020, 1180, level.end.x];
  return [
    ...deckRoad(xs, y),
    { ax: 180, ay: 520, bx: 380, by: y, material: 'steel' },
    { ax: 1432, ay: 520, bx: 1180, by: y, material: 'steel' },
    { ax: 380, ay: y, bx: 540, by: y + 100, material: 'steel' },
    { ax: 700, ay: y, bx: 540, by: y + 100, material: 'steel' },
    { ax: 700, ay: y, bx: 860, by: y + 100, material: 'steel' },
    { ax: 1020, ay: y, bx: 860, by: y + 100, material: 'steel' },
    { ax: 1020, ay: y, bx: 1180, by: y + 100, material: 'steel' },
    { ax: 540, ay: y, bx: 700, by: y + 100, material: 'wood' },
    { ax: 860, ay: y, bx: 1020, by: y + 100, material: 'wood' },
    { ax: 1180, ay: y, bx: 1020, by: y + 100, material: 'wood' },
  ];
}

const BUILDERS: Record<string, (level: LevelDef) => SpanSpec[]> = {
  knapford,
  tidmouth,
  hill,
  wellsworth,
  brendam,
  vicarstown,
  ffarquhar,
  express,
};

export function referenceSpans(level: LevelDef): SpanSpec[] {
  const builder = BUILDERS[level.id];
  if (!builder) {
    throw new Error(`No reference for ${level.id}`);
  }
  return builder(level);
}

export function buildReference(world: World, level: LevelDef): boolean {
  loadLevel(world, level);
  return buildSpans(world, referenceSpans(level));
}

export function buildAllReferences(): { id: string; ok: boolean; path: boolean; cost: number }[] {
  return LEVELS.map((level) => {
    const world = new World();
    const ok = buildReference(world, level);
    return {
      id: level.id,
      ok,
      path: world.hasRoadPath(),
      cost: world.totalCost(),
    };
  });
}
