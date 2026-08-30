import { MATERIALS, type MaterialId } from './materials';
import { dist, dist2, lerpVec, type Vec2 } from './vec';

export const SNAP_RADIUS = 18;
export const GRAVITY = 980;
export const DAMPING = 0.986;
export const SUBSTEPS = 8;
export const CONSTRAINT_ITERS = 10;
export const COACH_WEIGHT = 1.35;
export const TRAIN_BASE_FORCE = 72;

export type SimStatus = 'idle' | 'running' | 'success' | 'failed' | 'no_path';

export interface Node {
  id: number;
  x: number;
  y: number;
  px: number;
  py: number;
  mass: number;
  anchored: boolean;
  seeded: boolean;
}

export interface Beam {
  id: number;
  a: number;
  b: number;
  rest: number;
  material: MaterialId;
  broken: boolean;
  stress: number;
}

export interface TerrainPad {
  x0: number;
  x1: number;
  y: number;
}

export interface WorldSnapshot {
  nodes: Node[];
  beams: Beam[];
  nextNodeId: number;
  nextBeamId: number;
}

export interface SnapEvent {
  ax: number;
  ay: number;
  bx: number;
  by: number;
  age: number;
}

export interface TrainState {
  path: number[];
  pathIndex: number;
  t: number;
  x: number;
  y: number;
  vx: number;
  vy: number;
  falling: boolean;
  angle: number;
}

export interface BeamPreview {
  ok: boolean;
  reason: string;
  ax: number;
  ay: number;
  bx: number;
  by: number;
  length: number;
  cost: number;
  weak: boolean;
}

export type Edit =
  | { kind: 'beam'; beamId: number }
  | { kind: 'node'; nodeId: number; beamIds: number[] };

export class World {
  nodes = new Map<number, Node>();
  beams = new Map<number, Beam>();
  nextNodeId = 1;
  nextBeamId = 1;
  startId = 1;
  endId = 2;
  terrain: TerrainPad[] = [];
  waterY = 460;
  wind = 0;
  status: SimStatus = 'idle';
  train: TrainState | null = null;
  snapEvents: SnapEvent[] = [];
  edits: Edit[] = [];
  private saved: WorldSnapshot | null = null;

  clear(): void {
    this.nodes.clear();
    this.beams.clear();
    this.nextNodeId = 1;
    this.nextBeamId = 1;
    this.startId = 1;
    this.endId = 2;
    this.status = 'idle';
    this.train = null;
    this.snapEvents = [];
    this.edits = [];
    this.saved = null;
  }

  addNode(x: number, y: number, anchored = false, seeded = false): Node {
    const id = this.nextNodeId++;
    const node: Node = {
      id,
      x,
      y,
      px: x,
      py: y,
      mass: anchored ? 8 : 1.15,
      anchored,
      seeded,
    };
    this.nodes.set(id, node);
    return node;
  }

  node(id: number): Node | undefined {
    return this.nodes.get(id);
  }

  nearestNode(point: Vec2, radius = SNAP_RADIUS): Node | null {
    let best: Node | null = null;
    let bestD = radius * radius;
    for (const node of this.nodes.values()) {
      const d = dist2(node, point);
      if (d <= bestD) {
        best = node;
        bestD = d;
      }
    }
    return best;
  }

  ensureNode(x: number, y: number, anchored = false): Node {
    const existing = this.nearestNode({ x, y }, 12);
    if (existing) {
      return existing;
    }
    return this.addNode(x, y, anchored, false);
  }

  addBeam(aId: number, bId: number, material: MaterialId, record = true): Beam | null {
    const preview = this.previewBeam(aId, bId, material);
    if (!preview.ok) {
      return null;
    }
    const a = this.nodes.get(aId);
    const b = this.nodes.get(bId);
    if (!a || !b) {
      return null;
    }
    const beam: Beam = {
      id: this.nextBeamId++,
      a: aId,
      b: bId,
      rest: dist(a, b),
      material,
      broken: false,
      stress: 0,
    };
    this.beams.set(beam.id, beam);
    if (record) {
      this.edits.push({ kind: 'beam', beamId: beam.id });
    }
    return beam;
  }

  previewBeam(aId: number, bId: number, material: MaterialId): BeamPreview {
    const a = this.nodes.get(aId);
    const b = this.nodes.get(bId);
    const mat = MATERIALS[material];
    const empty: BeamPreview = {
      ok: false,
      reason: 'invalid',
      ax: 0,
      ay: 0,
      bx: 0,
      by: 0,
      length: 0,
      cost: 0,
      weak: false,
    };
    if (!a || !b) {
      return { ...empty, reason: 'missing joint' };
    }
    if (aId === bId) {
      return { ...empty, reason: 'same joint', ax: a.x, ay: a.y, bx: b.x, by: b.y };
    }
    const length = dist(a, b);
    if (length < mat.minLength) {
      return {
        ...empty,
        reason: 'too short',
        ax: a.x,
        ay: a.y,
        bx: b.x,
        by: b.y,
        length,
      };
    }
    if (length > mat.maxLength) {
      return {
        ...empty,
        reason: 'too long',
        ax: a.x,
        ay: a.y,
        bx: b.x,
        by: b.y,
        length,
        cost: length * mat.costPerPx,
        weak: true,
      };
    }
    if (this.findBeam(aId, bId)) {
      return {
        ...empty,
        reason: 'already connected',
        ax: a.x,
        ay: a.y,
        bx: b.x,
        by: b.y,
        length,
      };
    }
    return {
      ok: true,
      reason: '',
      ax: a.x,
      ay: a.y,
      bx: b.x,
      by: b.y,
      length,
      cost: length * mat.costPerPx,
      weak: false,
    };
  }

  findBeam(aId: number, bId: number): Beam | null {
    for (const beam of this.beams.values()) {
      if (beam.broken) {
        continue;
      }
      if ((beam.a === aId && beam.b === bId) || (beam.a === bId && beam.b === aId)) {
        return beam;
      }
    }
    return null;
  }

  nearestBeam(point: Vec2, radius = 14): Beam | null {
    let best: Beam | null = null;
    let bestD = radius;
    for (const beam of this.beams.values()) {
      if (beam.broken) {
        continue;
      }
      const a = this.nodes.get(beam.a);
      const b = this.nodes.get(beam.b);
      if (!a || !b) {
        continue;
      }
      const d = pointToSegment(point, a, b);
      if (d < bestD) {
        best = beam;
        bestD = d;
      }
    }
    return best;
  }

  removeBeam(id: number, record = false): boolean {
    const beam = this.beams.get(id);
    if (!beam) {
      return false;
    }
    this.beams.delete(id);
    if (record) {
      this.edits.push({ kind: 'beam', beamId: -id });
    }
    return true;
  }

  removePlayerNode(id: number): boolean {
    const node = this.nodes.get(id);
    if (!node || node.seeded || node.anchored) {
      return false;
    }
    const beamIds: number[] = [];
    for (const beam of [...this.beams.values()]) {
      if (beam.a === id || beam.b === id) {
        beamIds.push(beam.id);
        this.beams.delete(beam.id);
      }
    }
    this.nodes.delete(id);
    this.edits.push({ kind: 'node', nodeId: id, beamIds });
    return true;
  }

  undo(): boolean {
    const edit = this.edits.pop();
    if (!edit) {
      return false;
    }
    if (edit.kind === 'beam') {
      return this.beams.delete(edit.beamId);
    }
    this.nodes.delete(edit.nodeId);
    for (const beamId of edit.beamIds) {
      this.beams.delete(beamId);
    }
    return true;
  }

  liveBeams(): Beam[] {
    return [...this.beams.values()].filter((beam) => !beam.broken);
  }

  totalCost(): number {
    let cost = 0;
    for (const beam of this.liveBeams()) {
      cost += beam.rest * MATERIALS[beam.material].costPerPx;
    }
    return cost;
  }

  findPath(): number[] {
    const graph = new Map<number, number[]>();
    for (const node of this.nodes.keys()) {
      graph.set(node, []);
    }
    for (const beam of this.liveBeams()) {
      if (!MATERIALS[beam.material].traversable) {
        continue;
      }
      graph.get(beam.a)?.push(beam.b);
      graph.get(beam.b)?.push(beam.a);
    }

    const visited = new Set<number>([this.startId]);
    const prev = new Map<number, number>();
    const queue = [this.startId];
    while (queue.length > 0) {
      const cur = queue.shift();
      if (cur === undefined) {
        break;
      }
      if (cur === this.endId) {
        break;
      }
      for (const next of graph.get(cur) ?? []) {
        if (visited.has(next)) {
          continue;
        }
        visited.add(next);
        prev.set(next, cur);
        queue.push(next);
      }
    }

    if (!visited.has(this.endId)) {
      return [];
    }
    const path = [this.endId];
    while (path[0] !== this.startId) {
      const parent = prev.get(path[0]);
      if (parent === undefined) {
        return [];
      }
      path.unshift(parent);
    }
    return path;
  }

  hasRoadPath(): boolean {
    return this.findPath().length >= 2;
  }

  beginTest(engineWeight: number, engineSpeed: number, coaches: number): SimStatus {
    const path = this.findPath();
    if (path.length < 2) {
      this.status = 'no_path';
      return this.status;
    }
    this.saved = this.snapshot();
    this.status = 'running';
    const start = this.nodes.get(this.startId);
    this.train = {
      path,
      pathIndex: 0,
      t: 0,
      x: start?.x ?? 0,
      y: start?.y ?? 0,
      vx: 0,
      vy: 0,
      falling: false,
      angle: 0,
    };
    this._engineWeight = engineWeight + coaches * COACH_WEIGHT;
    this._engineSpeed = engineSpeed;
    return this.status;
  }

  endTest(): void {
    if (this.saved) {
      this.restore(this.saved);
    }
    this.status = 'idle';
    this.train = null;
    this.snapEvents = [];
  }

  step(dt: number): SimStatus {
    if (this.status !== 'running' || !this.train) {
      return this.status;
    }

    const slice = dt / SUBSTEPS;
    for (let i = 0; i < SUBSTEPS; i += 1) {
      this.integrate(slice);
      this.solveConstraints();
      this.applyTerrain();
      this.updateTrain(slice);
      if (this.status !== 'running') {
        break;
      }
    }

    for (const event of this.snapEvents) {
      event.age += dt;
    }
    this.snapEvents = this.snapEvents.filter((event) => event.age < 0.45);
    return this.status;
  }

  private _engineWeight = 4;
  private _engineSpeed = 1;

  private snapshot(): WorldSnapshot {
    return {
      nodes: [...this.nodes.values()].map((node) => ({ ...node })),
      beams: [...this.beams.values()].map((beam) => ({ ...beam })),
      nextNodeId: this.nextNodeId,
      nextBeamId: this.nextBeamId,
    };
  }

  private restore(data: WorldSnapshot): void {
    this.nodes.clear();
    this.beams.clear();
    for (const node of data.nodes) {
      this.nodes.set(node.id, { ...node, px: node.x, py: node.y });
    }
    for (const beam of data.beams) {
      this.beams.set(beam.id, { ...beam, broken: false, stress: 0 });
    }
    this.nextNodeId = data.nextNodeId;
    this.nextBeamId = data.nextBeamId;
  }

  private integrate(dt: number): void {
    const g = GRAVITY * dt * dt;
    const wind = this.wind * dt * dt;
    const load = this.currentLoad();
    for (const node of this.nodes.values()) {
      if (node.anchored) {
        node.px = node.x;
        node.py = node.y;
        continue;
      }
      const extraY = load.get(node.id) ?? 0;
      const nx = node.x + (node.x - node.px) * DAMPING + wind;
      const ny = node.y + (node.y - node.py) * DAMPING + g + extraY * dt * dt;
      node.px = node.x;
      node.py = node.y;
      node.x = nx;
      node.y = ny;
    }
  }

  private currentLoad(): Map<number, number> {
    const map = new Map<number, number>();
    const train = this.train;
    if (!train || train.falling) {
      return map;
    }
    const a = this.nodes.get(train.path[train.pathIndex]);
    const b = this.nodes.get(train.path[train.pathIndex + 1]);
    if (!a || !b) {
      return map;
    }
    const force = this._engineWeight * TRAIN_BASE_FORCE;
    map.set(a.id, force * (1 - train.t) / a.mass);
    map.set(b.id, force * train.t / b.mass);
    return map;
  }

  private solveConstraints(): void {
    for (let iter = 0; iter < CONSTRAINT_ITERS; iter += 1) {
      for (const beam of this.liveBeams()) {
        const a = this.nodes.get(beam.a);
        const b = this.nodes.get(beam.b);
        if (!a || !b) {
          continue;
        }
        const dx = b.x - a.x;
        const dy = b.y - a.y;
        const length = Math.hypot(dx, dy) || 0.0001;
        const mat = MATERIALS[beam.material];
        const diff = (length - beam.rest) / length;
        const strain = Math.abs(length - beam.rest) / Math.max(beam.rest, 1);
        beam.stress = Math.min(1, strain / mat.maxStrain);
        if (strain > mat.maxStrain) {
          this.breakBeam(beam);
          continue;
        }
        const stiffness = mat.stiffness;
        const invA = a.anchored ? 0 : 1 / a.mass;
        const invB = b.anchored ? 0 : 1 / b.mass;
        const inv = invA + invB;
        if (inv <= 0) {
          continue;
        }
        const corr = diff * stiffness;
        const ox = dx * corr;
        const oy = dy * corr;
        if (!a.anchored) {
          a.x += ox * (invA / inv);
          a.y += oy * (invA / inv);
        }
        if (!b.anchored) {
          b.x -= ox * (invB / inv);
          b.y -= oy * (invB / inv);
        }
      }
    }
  }

  private breakBeam(beam: Beam): void {
    if (beam.broken) {
      return;
    }
    beam.broken = true;
    const a = this.nodes.get(beam.a);
    const b = this.nodes.get(beam.b);
    if (a && b) {
      this.snapEvents.push({ ax: a.x, ay: a.y, bx: b.x, by: b.y, age: 0 });
    }
    const train = this.train;
    if (train && !train.falling) {
      const aId = train.path[train.pathIndex];
      const bId = train.path[train.pathIndex + 1];
      if ((beam.a === aId && beam.b === bId) || (beam.a === bId && beam.b === aId)) {
        this.startFall();
      } else if (!this.pathStillValid(train.path, train.pathIndex)) {
        this.startFall();
      }
    }
  }

  private pathStillValid(path: number[], fromIndex: number): boolean {
    for (let i = fromIndex; i < path.length - 1; i += 1) {
      if (!this.findBeam(path[i], path[i + 1])) {
        return false;
      }
    }
    return true;
  }

  private applyTerrain(): void {
    for (const node of this.nodes.values()) {
      if (node.anchored) {
        continue;
      }
      const ground = this.groundAt(node.x);
      if (ground !== null && node.y > ground - 6) {
        node.y = ground - 6;
        node.py = node.y;
      }
    }
  }

  groundAt(x: number): number | null {
    for (const pad of this.terrain) {
      if (x >= pad.x0 && x <= pad.x1) {
        return pad.y;
      }
    }
    return null;
  }

  private updateTrain(dt: number): void {
    const train = this.train;
    if (!train) {
      return;
    }

    if (train.falling) {
      train.vy += GRAVITY * dt;
      train.x += train.vx * dt;
      train.y += train.vy * dt;
      train.angle += dt * 1.8;
      if (train.y > this.waterY + 80) {
        this.status = 'failed';
      }
      return;
    }

    const a = this.nodes.get(train.path[train.pathIndex]);
    const b = this.nodes.get(train.path[train.pathIndex + 1]);
    if (!a || !b) {
      this.startFall();
      return;
    }

    const length = dist(a, b);
    const speed = 165 * this._engineSpeed;
    train.t += (speed * dt) / Math.max(length, 1);
    if (train.t >= 1) {
      train.t = 0;
      train.pathIndex += 1;
      if (train.pathIndex >= train.path.length - 1) {
        const end = this.nodes.get(this.endId);
        if (end) {
          train.x = end.x;
          train.y = end.y;
        }
        this.status = 'success';
        return;
      }
    }

    const pos = lerpVec(a, b, train.t);
    train.x = pos.x;
    train.y = pos.y - 10;
    train.angle = Math.atan2(b.y - a.y, b.x - a.x);
    train.vx = ((b.x - a.x) / Math.max(length, 1)) * speed;
    train.vy = ((b.y - a.y) / Math.max(length, 1)) * speed;

    if (train.y > this.waterY + 24) {
      this.startFall();
    }
  }

  private startFall(): void {
    if (!this.train) {
      return;
    }
    this.train.falling = true;
    this.train.vy = 40;
  }
}

function pointToSegment(p: Vec2, a: Vec2, b: Vec2): number {
  const dx = b.x - a.x;
  const dy = b.y - a.y;
  const len2 = dx * dx + dy * dy;
  if (len2 <= 0.001) {
    return dist(p, a);
  }
  let t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / len2;
  t = Math.max(0, Math.min(1, t));
  return Math.hypot(p.x - (a.x + dx * t), p.y - (a.y + dy * t));
}

export function runCrossing(
  world: World,
  weight: number,
  speed: number,
  coaches: number,
  seconds = 18,
): SimStatus {
  world.beginTest(weight, speed, coaches);
  const dt = 1 / 60;
  const frames = Math.ceil(seconds * 60);
  for (let i = 0; i < frames; i += 1) {
    const status = world.step(dt);
    if (status === 'success' || status === 'failed' || status === 'no_path') {
      return status;
    }
  }
  return world.status;
}
