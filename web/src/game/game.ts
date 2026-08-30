import { AudioDirector } from './audio';
import { engineById, type EngineId } from './engines';
import { loadLevel } from './level-io';
import { LEVELS, type LevelDef } from './levels';
import { MATERIALS, spanIsWeak, type MaterialId } from './materials';
import { COACH_WEIGHT, World, type SimStatus } from './physics';
import {
  isEngineUnlocked,
  isLevelUnlocked,
  loadProgress,
  recordClear,
  saveProgress,
  type Progress,
} from './progress';
import {
  drawFrame,
  fitCamera,
  resizeCanvas,
  screenToWorld,
  type Camera,
  type DrawPreview,
} from './renderer';
import { formatCost, starsForCost } from './score';
import { buildReference } from './solutions';

export type ScreenId = 'title' | 'levels' | 'engines' | 'build' | 'result';
export type ToolId = MaterialId | 'erase';

export interface GameHooks {
  getScreen(): ScreenId;
  getLevelId(): string;
  getStatus(): SimStatus;
  getCost(): number;
  getPendingCost(): number;
  hasPath(): boolean;
  getStars(): number;
  getTool(): ToolId;
  selectLevel(index: number): void;
  selectEngine(id: EngineId): void;
  selectMaterial(id: ToolId): void;
  startTest(): SimStatus;
  cancelTest(): void;
  buildReference(): boolean;
  goTitle(): void;
  goLevels(): void;
  goBuild(): void;
  snapshot(): Record<string, unknown>;
  worldToScreen(x: number, y: number): { x: number; y: number };
  weld(ax: number, ay: number, bx: number, by: number): boolean;
}

export class BridgeGame {
  readonly world = new World();
  screen: ScreenId = 'title';
  levelIndex = 0;
  engineId: EngineId = 'thomas';
  tool: ToolId = 'road';
  progress: Progress = loadProgress();
  lastStars = 0;
  lastStatus: SimStatus = 'idle';
  toast = '';
  ghostCost = 0;
  tutorialStep = 0;
  camera: Camera = { x: 0, y: 0, zoom: 1 };
  viewW = 1280;
  viewH = 720;

  private canvas: HTMLCanvasElement;
  private dragging = false;
  private fromId: number | null = null;
  private pointer = { x: 0, y: 0 };
  private hoverNode: number | null = null;
  private hoverBeam: number | null = null;
  private time = 0;
  private raf = 0;
  private audio = new AudioDirector();
  private resultDelay = 0;

  constructor(canvas: HTMLCanvasElement) {
    this.canvas = canvas;
    loadLevel(this.world, this.level);
    this.bind();
  }

  engineLoad(): number {
    const engine = engineById(this.engineId);
    return engine.weight + engine.coaches * COACH_WEIGHT;
  }

  get level(): LevelDef {
    return LEVELS[this.levelIndex];
  }

  start(): void {
    this.fit();
    this.loop(performance.now());
  }

  stop(): void {
    cancelAnimationFrame(this.raf);
  }

  goTitle(): void {
    this.world.endTest();
    this.levelIndex = 0;
    loadLevel(this.world, this.level);
    this.screen = 'title';
    this.fit();
  }

  goLevels(): void {
    this.world.endTest();
    this.screen = 'levels';
  }

  goEngines(index = this.levelIndex): void {
    if (!isLevelUnlocked(this.progress, index)) {
      this.showToast('Finish the previous crossing first.');
      return;
    }
    this.levelIndex = index;
    this.world.endTest();
    loadLevel(this.world, this.level);
    this.screen = 'engines';
    if (!isEngineUnlocked(this.progress, this.engineId)) {
      this.engineId = 'thomas';
    }
    this.fit();
  }

  goBuild(): void {
    loadLevel(this.world, this.level);
    this.tool = 'road';
    this.fromId = null;
    this.dragging = false;
    this.tutorialStep = this.progress.seenTutorial ? 99 : 0;
    this.screen = 'build';
    this.fit();
  }

  selectEngine(id: EngineId): void {
    if (!isEngineUnlocked(this.progress, id)) {
      this.showToast('That engine is still in the shed.');
      return;
    }
    this.engineId = id;
  }

  selectTool(tool: ToolId): void {
    if (tool !== 'erase' && !this.level.tools.includes(tool)) {
      this.showToast('That material is not available on this crossing.');
      return;
    }
    this.tool = tool;
    this.fromId = null;
    this.audio.ui();
  }

  startTest(): SimStatus {
    if (this.world.status === 'running') {
      return this.world.status;
    }
    if (!this.world.hasRoadPath()) {
      this.showToast('Connect a continuous road from START to END.');
      this.world.status = 'no_path';
      return 'no_path';
    }
    const engine = engineById(this.engineId);
    this.resultDelay = 0;
    const status = this.world.beginTest(engine.weight, engine.speed, engine.coaches);
    this.lastStatus = status;
    if (status === 'running') {
      this.tutorialStep = 99;
    }
    return status;
  }

  cancelTest(): void {
    this.world.endTest();
    this.resultDelay = 0;
  }

  applyReference(): boolean {
    const ok = buildReference(this.world, this.level);
    this.fromId = null;
    return ok;
  }

  hooks(): GameHooks {
    return {
      getScreen: () => this.screen,
      getLevelId: () => this.level.id,
      getStatus: () => this.world.status,
      getCost: () => this.world.totalCost(),
      getPendingCost: () => this.ghostCost,
      hasPath: () => this.world.hasRoadPath(),
      getStars: () => this.lastStars,
      getTool: () => this.tool,
      selectLevel: (index) => this.goEngines(index),
      selectEngine: (id) => this.selectEngine(id),
      selectMaterial: (id) => this.selectTool(id),
      startTest: () => this.startTest(),
      cancelTest: () => this.cancelTest(),
      buildReference: () => this.applyReference(),
      goTitle: () => this.goTitle(),
      goLevels: () => this.goLevels(),
      goBuild: () => this.goBuild(),
      snapshot: () => ({
        screen: this.screen,
        level: this.level.id,
        engine: this.engineId,
        status: this.world.status,
        cost: this.world.totalCost(),
        pending: this.ghostCost,
        load: this.engineLoad(),
        nodes: this.world.nodes.size,
        beams: this.world.liveBeams().length,
        path: this.world.hasRoadPath(),
        stars: this.lastStars,
      }),
      worldToScreen: (x, y) => ({
        x: (x - this.camera.x) * this.camera.zoom,
        y: (y - this.camera.y) * this.camera.zoom,
      }),
      weld: (ax, ay, bx, by) => {
        const from = this.world.nearestNode({ x: ax, y: ay }, 24);
        if (!from || this.tool === 'erase') {
          return false;
        }
        this.fromId = from.id;
        this.finishBeam({ x: bx, y: by });
        this.fromId = null;
        return this.world.liveBeams().length > 0;
      },
    };
  }

  private bind(): void {
    window.addEventListener('resize', () => this.fit());
    this.canvas.addEventListener('pointerdown', (event) => this.onDown(event));
    this.canvas.addEventListener('pointermove', (event) => this.onMove(event));
    window.addEventListener('pointerup', (event) => this.onUp(event));
    this.canvas.addEventListener('contextmenu', (event) => event.preventDefault());
    window.addEventListener('keydown', (event) => this.onKey(event));
  }

  private fit(): void {
    const size = resizeCanvas(this.canvas);
    this.viewW = size.width;
    this.viewH = size.height;
    this.camera = fitCamera(this.level, this.viewW, this.viewH);
  }

  private worldPoint(event: PointerEvent) {
    return screenToWorld(this.camera, event.clientX, event.clientY, this.canvas.getBoundingClientRect());
  }

  private onDown(event: PointerEvent): void {
    if (this.screen !== 'build' || this.world.status === 'running') {
      return;
    }
    this.audio.resume();
    this.canvas.setPointerCapture(event.pointerId);
    const point = this.worldPoint(event);
    this.pointer = point;
    if (this.tool === 'erase') {
      const beam = this.world.nearestBeam(point);
      if (beam) {
        this.world.removeBeam(beam.id);
        this.audio.ui();
        return;
      }
      const node = this.world.nearestNode(point);
      if (node) {
        this.world.removePlayerNode(node.id);
        this.audio.ui();
      }
      return;
    }
    const snapped = this.world.nearestNode(point);
    if (!snapped) {
      this.showToast('Start from an existing joint — weld from a bank or a beam you already placed.');
      return;
    }
    this.fromId = snapped.id;
    this.dragging = true;
    this.advanceTutorial(1);
  }

  private onMove(event: PointerEvent): void {
    if (this.screen !== 'build') {
      return;
    }
    const point = this.worldPoint(event);
    this.pointer = point;
    this.hoverNode = this.world.nearestNode(point)?.id ?? null;
    this.hoverBeam = this.world.nearestBeam(point)?.id ?? null;
    if (this.dragging) {
      this.chainToward(point);
    }
  }

  private onUp(event: PointerEvent): void {
    if (!this.dragging || this.fromId === null) {
      this.dragging = false;
      return;
    }
    const point = this.worldPoint(event);
    this.chainToward(point);
    this.finishBeam(point, true);
    this.dragging = false;
    this.fromId = null;
  }

  private chainToward(point: { x: number; y: number }): void {
    if (this.fromId === null || this.tool === 'erase') {
      return;
    }
    const material = this.tool;
    const plantAt = MATERIALS[material].maxLength * 0.78;
    for (let guard = 0; guard < 8; guard += 1) {
      const from = this.world.node(this.fromId);
      if (!from) {
        return;
      }
      const hover = this.world.nearestNode(point);
      if (hover && hover.id !== this.fromId) {
        const reach = Math.hypot(hover.x - from.x, hover.y - from.y);
        if (reach <= MATERIALS[material].maxLength) {
          return;
        }
      }
      const length = Math.hypot(point.x - from.x, point.y - from.y);
      if (length < plantAt) {
        return;
      }
      const t = plantAt / length;
      const planted = this.finishBeam(
        { x: from.x + (point.x - from.x) * t, y: from.y + (point.y - from.y) * t },
        false,
      );
      if (!planted) {
        return;
      }
    }
  }

  private finishBeam(point: { x: number; y: number }, announce = true): boolean {
    if (this.fromId === null || this.tool === 'erase') {
      return false;
    }
    const material = this.tool;
    let target = this.world.nearestNode(point);
    if (!target) {
      const from = this.world.node(this.fromId);
      if (!from) {
        return false;
      }
      const length = Math.hypot(point.x - from.x, point.y - from.y);
      if (length < MATERIALS[material].minLength) {
        if (announce) {
          this.showToast('Drag farther to weld a span.');
        }
        return false;
      }
      if (length > MATERIALS[material].maxLength) {
        if (announce) {
          this.showToast('That span is too long. Place a joint in between.');
          this.audio.fail();
        }
        return false;
      }
      target = this.world.addNode(point.x, point.y, false, false);
    }
    const preview = this.world.previewBeam(this.fromId, target.id, material);
    if (!preview.ok) {
      if (announce) {
        this.showToast(this.humanReason(preview.reason));
      }
      if (target && !target.seeded && this.world.liveBeams().every((beam) => beam.a !== target.id && beam.b !== target.id)) {
        this.world.nodes.delete(target.id);
      }
      return false;
    }
    if (this.world.totalCost() + preview.cost > this.level.budget) {
      if (announce) {
        this.showToast('That weld would blow the budget.');
      }
      if (target && !target.seeded && this.world.liveBeams().every((beam) => beam.a !== target.id && beam.b !== target.id)) {
        this.world.nodes.delete(target.id);
      }
      return false;
    }
    const beam = this.world.addBeam(this.fromId, target.id, material);
    if (beam) {
      this.audio.place();
      this.fromId = target.id;
      if (this.world.hasRoadPath()) {
        this.advanceTutorial(3);
      } else {
        this.advanceTutorial(2);
      }
      return true;
    }
    return false;
  }

  private humanReason(reason: string): string {
    switch (reason) {
      case 'too long':
        return 'Too long to weld. Break it into shorter spans.';
      case 'too short':
        return 'A little farther — that is still the same joint.';
      case 'already connected':
        return 'Those joints are already welded.';
      default:
        return 'Cannot place that span.';
    }
  }

  private onKey(event: KeyboardEvent): void {
    if (event.target instanceof HTMLInputElement || event.target instanceof HTMLTextAreaElement) {
      return;
    }
    if (event.key === 'Escape') {
      if (this.world.status === 'running') {
        this.cancelTest();
      } else if (this.screen === 'build') {
        this.goLevels();
      } else if (this.screen !== 'title') {
        this.goTitle();
      }
      return;
    }
    if (this.screen !== 'build') {
      return;
    }
    if (event.key === '1') {
      this.selectTool('road');
    } else if (event.key === '2') {
      this.selectTool('wood');
    } else if (event.key === '3' && this.level.tools.includes('steel')) {
      this.selectTool('steel');
    } else if (event.key === 'e' || event.key === 'E') {
      this.selectTool('erase');
    } else if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === 'z') {
      event.preventDefault();
      this.world.undo();
    } else if (event.key === 'Enter' || event.key === ' ') {
      event.preventDefault();
      this.startTest();
    } else if (event.key === 'u' || event.key === 'U') {
      this.world.undo();
    }
  }

  private advanceTutorial(step: number): void {
    if (this.progress.seenTutorial) {
      return;
    }
    this.tutorialStep = Math.max(this.tutorialStep, step);
    if (step >= 3) {
      this.progress = { ...this.progress, seenTutorial: true };
      saveProgress(this.progress);
    }
  }

  showToast(message: string): void {
    this.toast = message;
    window.setTimeout(() => {
      if (this.toast === message) {
        this.toast = '';
      }
    }, 2400);
  }

  private loop = (now: number): void => {
    const dt = Math.min(0.033, (now - (this.time || now)) / 1000 || 0.016);
    this.time = now / 1000;
    if (this.screen === 'build') {
      if (this.world.status === 'running') {
        const before = this.world.snapEvents.length;
        this.world.step(dt);
        if (this.world.snapEvents.length > before) {
          this.audio.snap();
        }
      }
      if (this.world.status === 'success' || this.world.status === 'failed') {
        this.resultDelay += dt;
        if (this.resultDelay > 0.55) {
          this.finishRun();
        }
      }
    }

    const ctx = this.canvas.getContext('2d');
    if (ctx) {
      const preview = this.preview();
      this.ghostCost = preview?.cost ?? 0;
      drawFrame(ctx, this.viewW, this.viewH, {
        world: this.world,
        level: this.level,
        camera: this.camera,
        time: this.time,
        engine: engineById(this.engineId),
        load: this.engineLoad(),
        preview,
        hoverNode: this.hoverNode,
        hoverBeam: this.hoverBeam,
        tool: this.tool,
        testing: this.world.status === 'running',
      });
    }
    this.raf = requestAnimationFrame(this.loop);
  };

  private finishRun(): void {
    this.lastStatus = this.world.status;
    if (this.world.status === 'success') {
      this.lastStars = starsForCost(this.world.totalCost(), this.level.parCost);
      this.progress = recordClear(this.progress, this.levelIndex, this.lastStars, this.world.totalCost());
      saveProgress(this.progress);
      this.audio.success();
    } else {
      this.lastStars = 0;
      this.audio.fail();
    }
    this.screen = 'result';
    this.resultDelay = 0;
  }

  private preview(): DrawPreview | null {
    if (!this.dragging || this.fromId === null || this.tool === 'erase') {
      return this.fromId !== null
        ? {
            ax: 0,
            ay: 0,
            bx: 0,
            by: 0,
            material: 'road',
            ok: true,
            weak: false,
            cost: 0,
            selectedId: this.fromId,
          }
        : null;
    }
    const from = this.world.node(this.fromId);
    if (!from) {
      return null;
    }
    const hover = this.world.nearestNode(this.pointer);
    const to = hover ?? this.pointer;
    const length = Math.hypot(to.x - from.x, to.y - from.y);
    const preview = hover
      ? this.world.previewBeam(this.fromId, hover.id, this.tool)
      : {
          ok: distOk(from, to, this.tool),
          reason: '',
          ax: from.x,
          ay: from.y,
          bx: to.x,
          by: to.y,
          length,
          cost: length * MATERIALS[this.tool].costPerPx,
          weak: spanIsWeak(length, this.tool, this.engineLoad()) || !distOk(from, to, this.tool),
        };
    return {
      ax: from.x,
      ay: from.y,
      bx: to.x,
      by: to.y,
      material: this.tool,
      ok: preview.ok,
      weak: preview.weak || !preview.ok,
      cost: preview.ok ? preview.cost : 0,
      selectedId: this.fromId,
    };
  }
}

function distOk(from: { x: number; y: number }, to: { x: number; y: number }, material: MaterialId): boolean {
  const length = Math.hypot(to.x - from.x, to.y - from.y);
  const mat = MATERIALS[material];
  return length >= mat.minLength && length <= mat.maxLength;
}

export { formatCost };
