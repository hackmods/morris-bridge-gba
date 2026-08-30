import type { LevelDef } from './levels';
import type { MaterialId } from './materials';
import { World } from './physics';

export function loadLevel(world: World, def: LevelDef): void {
  world.clear();
  world.terrain = def.terrain.map((pad) => ({ ...pad }));
  world.waterY = def.waterY;
  world.wind = def.wind;
  for (const anchor of def.anchors) {
    world.addNode(anchor.x, anchor.y, true, true);
  }
  world.startId = 1;
  world.endId = 2;
}

export interface SpanSpec {
  ax: number;
  ay: number;
  bx: number;
  by: number;
  material: MaterialId;
}

export function buildSpans(world: World, spans: SpanSpec[]): boolean {
  for (const span of spans) {
    const a = world.ensureNode(span.ax, span.ay, false);
    const b = world.ensureNode(span.bx, span.by, false);
    if (!world.addBeam(a.id, b.id, span.material, false)) {
      return false;
    }
  }
  return true;
}

export function deckRoad(xs: number[], y: number): SpanSpec[] {
  const spans: SpanSpec[] = [];
  for (let i = 0; i < xs.length - 1; i += 1) {
    spans.push({
      ax: xs[i],
      ay: y,
      bx: xs[i + 1],
      by: y,
      material: 'road',
    });
  }
  return spans;
}
