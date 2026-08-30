import { type EngineProfile } from './engines';
import type { LevelDef } from './levels';
import { MATERIALS, type MaterialId } from './materials';
import type { Beam, Node, TrainState, World } from './physics';
import { clamp, dist, mixColor } from './vec';

export interface Camera {
  x: number;
  y: number;
  zoom: number;
}

export interface PointerWorld {
  x: number;
  y: number;
}

export interface DrawPreview {
  ax: number;
  ay: number;
  bx: number;
  by: number;
  material: MaterialId;
  ok: boolean;
  weak: boolean;
  selectedId: number | null;
}

export interface RenderFrame {
  world: World;
  level: LevelDef;
  camera: Camera;
  time: number;
  engine: EngineProfile;
  preview: DrawPreview | null;
  hoverNode: number | null;
  hoverBeam: number | null;
  tool: MaterialId | 'erase';
  testing: boolean;
}

export function fitCamera(level: LevelDef, viewW: number, viewH: number): Camera {
  const pad = 48;
  const zoom = Math.min((viewW - pad * 2) / level.width, (viewH - pad * 2) / level.height);
  return {
    x: (level.width - viewW / zoom) / 2,
    y: (level.height - viewH / zoom) / 2 - 18,
    zoom: Math.max(0.42, zoom),
  };
}

export function screenToWorld(
  camera: Camera,
  clientX: number,
  clientY: number,
  bounds: DOMRect,
): PointerWorld {
  const x = (clientX - bounds.left) / camera.zoom + camera.x;
  const y = (clientY - bounds.top) / camera.zoom + camera.y;
  return { x, y };
}

export function resizeCanvas(canvas: HTMLCanvasElement): { width: number; height: number } {
  const dpr = Math.min(2, window.devicePixelRatio || 1);
  const rect = canvas.getBoundingClientRect();
  const width = Math.max(1, Math.round(rect.width));
  const height = Math.max(1, Math.round(rect.height));
  canvas.width = Math.round(width * dpr);
  canvas.height = Math.round(height * dpr);
  const ctx = canvas.getContext('2d');
  if (ctx) {
    ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
  }
  return { width, height };
}

export function drawFrame(ctx: CanvasRenderingContext2D, viewW: number, viewH: number, frame: RenderFrame): void {
  const { camera, level, world, time } = frame;
  ctx.clearRect(0, 0, viewW, viewH);
  ctx.save();
  drawSky(ctx, viewW, viewH, level, time);
  ctx.translate(-camera.x * camera.zoom, -camera.y * camera.zoom);
  ctx.scale(camera.zoom, camera.zoom);
  drawHills(ctx, level, time);
  drawWater(ctx, level, time);
  drawTerrain(ctx, level);
  if (!frame.testing) {
    drawGuide(ctx, world);
  }
  drawBeams(ctx, world, frame);
  if (frame.preview) {
    drawPreview(ctx, frame.preview);
  }
  drawJoints(ctx, world, frame);
  drawSnap(ctx, world);
  if (world.train) {
    drawTrain(ctx, world.train, frame.engine, time);
  } else {
    drawParkedEngine(ctx, world, frame.engine);
  }
  ctx.restore();
  drawVignette(ctx, viewW, viewH);
}

function drawSky(
  ctx: CanvasRenderingContext2D,
  w: number,
  h: number,
  level: LevelDef,
  time: number,
): void {
  const g = ctx.createLinearGradient(0, 0, 0, h);
  g.addColorStop(0, level.theme.skyTop);
  g.addColorStop(0.55, level.theme.skyHorizon);
  g.addColorStop(1, '#f6efe2');
  ctx.fillStyle = g;
  ctx.fillRect(0, 0, w, h);

  const sunX = w * 0.78;
  const sunY = h * 0.18 + Math.sin(time * 0.15) * 4;
  const sun = ctx.createRadialGradient(sunX, sunY, 8, sunX, sunY, 140);
  sun.addColorStop(0, level.theme.sun);
  sun.addColorStop(1, 'rgba(255, 230, 180, 0)');
  ctx.fillStyle = sun;
  ctx.beginPath();
  ctx.arc(sunX, sunY, 140, 0, Math.PI * 2);
  ctx.fill();

  ctx.fillStyle = 'rgba(255,255,255,0.55)';
  drawCloud(ctx, w * 0.18, h * 0.14, 46, time * 6);
  ctx.fillStyle = 'rgba(255,255,255,0.4)';
  drawCloud(ctx, w * 0.58, h * 0.1, 36, time * 4);
}

function drawCloud(ctx: CanvasRenderingContext2D, x: number, y: number, r: number, drift: number): void {
  const dx = (drift % 800) - 40;
  ctx.beginPath();
  ctx.ellipse(x + dx, y, r * 1.6, r * 0.55, 0, 0, Math.PI * 2);
  ctx.ellipse(x + dx + r * 0.7, y - 8, r, r * 0.5, 0, 0, Math.PI * 2);
  ctx.ellipse(x + dx - r * 0.6, y - 4, r * 0.9, r * 0.45, 0, 0, Math.PI * 2);
  ctx.fill();
}

function drawHills(ctx: CanvasRenderingContext2D, level: LevelDef, time: number): void {
  const w = level.width;
  const h = level.height;
  ctx.fillStyle = level.theme.farHill;
  ctx.beginPath();
  ctx.moveTo(-80, h);
  ctx.lineTo(-80, h * 0.58);
  for (let x = 0; x <= w; x += 40) {
    const y = h * 0.5 + Math.sin(x * 0.006 + 0.4) * 28 + Math.sin(x * 0.02) * 10;
    ctx.lineTo(x, y);
  }
  ctx.lineTo(w + 80, h);
  ctx.closePath();
  ctx.fill();

  ctx.fillStyle = level.theme.midHill;
  ctx.beginPath();
  ctx.moveTo(-80, h);
  ctx.lineTo(-80, h * 0.66);
  for (let x = 0; x <= w; x += 36) {
    const y = h * 0.58 + Math.sin(x * 0.01 + time * 0.05) * 18;
    ctx.lineTo(x, y);
  }
  ctx.lineTo(w + 80, h);
  ctx.closePath();
  ctx.fill();
}

function drawWater(ctx: CanvasRenderingContext2D, level: LevelDef, time: number): void {
  const y = level.waterY;
  const w = level.width;
  const g = ctx.createLinearGradient(0, y - 30, 0, level.height);
  g.addColorStop(0, level.theme.water);
  g.addColorStop(1, level.theme.waterDeep);
  ctx.fillStyle = g;
  ctx.fillRect(-100, y - 8, w + 200, level.height - y + 40);

  ctx.strokeStyle = 'rgba(255,255,255,0.28)';
  ctx.lineWidth = 2;
  for (let i = 0; i < 5; i += 1) {
    ctx.beginPath();
    const yy = y + 10 + i * 14;
    for (let x = 0; x <= w; x += 16) {
      const wave = Math.sin(x * 0.02 + time * 1.4 + i) * 3;
      if (x === 0) {
        ctx.moveTo(x, yy + wave);
      } else {
        ctx.lineTo(x, yy + wave);
      }
    }
    ctx.stroke();
  }
}

function drawTerrain(ctx: CanvasRenderingContext2D, level: LevelDef): void {
  for (const pad of level.terrain) {
    const width = pad.x1 - pad.x0;
    ctx.fillStyle = level.theme.dirtDark;
    ctx.fillRect(pad.x0, pad.y, width, level.height - pad.y + 40);
    ctx.fillStyle = level.theme.dirt;
    ctx.beginPath();
    ctx.moveTo(pad.x0, pad.y + 18);
    ctx.lineTo(pad.x0, pad.y);
    ctx.lineTo(pad.x1, pad.y);
    ctx.lineTo(pad.x1, pad.y + 18);
    ctx.closePath();
    ctx.fill();
    ctx.fillStyle = level.theme.grass;
    ctx.fillRect(pad.x0, pad.y - 10, width, 16);
    ctx.fillStyle = level.theme.grassDark;
    for (let x = pad.x0 + 10; x < pad.x1; x += 18) {
      ctx.beginPath();
      ctx.moveTo(x, pad.y - 8);
      ctx.lineTo(x + 4, pad.y - 18);
      ctx.lineTo(x + 8, pad.y - 8);
      ctx.fill();
    }
  }
}

function drawGuide(ctx: CanvasRenderingContext2D, world: World): void {
  const start = world.node(world.startId);
  const end = world.node(world.endId);
  if (!start || !end) {
    return;
  }
  ctx.save();
  ctx.setLineDash([10, 10]);
  ctx.strokeStyle = 'rgba(255,255,255,0.35)';
  ctx.lineWidth = 3;
  ctx.beginPath();
  ctx.moveTo(start.x, start.y);
  ctx.lineTo(end.x, end.y);
  ctx.stroke();
  ctx.restore();

  ctx.font = '600 16px "Source Sans 3", system-ui, sans-serif';
  ctx.fillStyle = 'rgba(28,42,56,0.7)';
  ctx.fillText('START', start.x - 24, start.y - 28);
  ctx.fillText('END', end.x - 16, end.y - 28);
}

function drawBeams(ctx: CanvasRenderingContext2D, world: World, frame: RenderFrame): void {
  const beams = [...world.beams.values()].sort((left, right) => {
    const order = (id: Beam['material']) => (id === 'road' ? 1 : 0);
    return order(left.material) - order(right.material);
  });
  for (const beam of beams) {
    const a = world.node(beam.a);
    const b = world.node(beam.b);
    if (!a || !b) {
      continue;
    }
    const hover = frame.hoverBeam === beam.id;
    drawBeam(ctx, a, b, beam, frame.testing || world.status === 'running', hover);
  }
}

function drawBeam(
  ctx: CanvasRenderingContext2D,
  a: Node,
  b: Node,
  beam: Beam,
  showStress: boolean,
  hover: boolean,
): void {
  const mat = MATERIALS[beam.material];
  const angle = Math.atan2(b.y - a.y, b.x - a.x);
  const length = dist(a, b);
  const width = beam.material === 'road' ? 18 : beam.material === 'steel' ? 8 : 11;
  ctx.save();
  ctx.translate(a.x, a.y);
  ctx.rotate(angle);
  ctx.globalAlpha = beam.broken ? 0.25 : 1;

  ctx.fillStyle = 'rgba(20,24,30,0.18)';
  ctx.fillRect(0, 4, length, width);

  let fill = mat.color;
  if (showStress) {
    fill = mixColor(mat.color, '#e23b2f', clamp(beam.stress * 1.15, 0, 1));
  }
  ctx.fillStyle = fill;
  roundedRect(ctx, 0, -width / 2, length, width, 4);
  ctx.fill();

  if (beam.material === 'road') {
    ctx.fillStyle = mat.colorLight;
    ctx.fillRect(8, -1.5, Math.max(0, length - 16), 3);
    ctx.fillStyle = 'rgba(0,0,0,0.25)';
    ctx.fillRect(0, -width / 2, length, 2);
    ctx.fillRect(0, width / 2 - 2, length, 2);
  } else if (beam.material === 'wood') {
    ctx.strokeStyle = mat.colorDark;
    ctx.globalAlpha = beam.broken ? 0.2 : 0.35;
    ctx.lineWidth = 1;
    for (let x = 8; x < length; x += 14) {
      ctx.beginPath();
      ctx.moveTo(x, -width / 2 + 1);
      ctx.lineTo(x - 4, width / 2 - 1);
      ctx.stroke();
    }
  } else {
    ctx.strokeStyle = mat.colorLight;
    ctx.globalAlpha = 0.55;
    ctx.lineWidth = 2;
    ctx.beginPath();
    ctx.moveTo(4, 0);
    ctx.lineTo(length - 4, 0);
    ctx.stroke();
  }

  if (hover) {
    ctx.globalAlpha = 1;
    ctx.strokeStyle = '#fff6d8';
    ctx.lineWidth = 2;
    roundedRect(ctx, -2, -width / 2 - 2, length + 4, width + 4, 6);
    ctx.stroke();
  }
  ctx.restore();
}

function drawPreview(ctx: CanvasRenderingContext2D, preview: DrawPreview): void {
  const mat = MATERIALS[preview.material];
  ctx.save();
  ctx.setLineDash(preview.ok ? [] : [8, 6]);
  ctx.strokeStyle = preview.weak || !preview.ok ? '#e35b4a' : mat.colorLight;
  ctx.lineWidth = preview.material === 'road' ? 10 : 7;
  ctx.lineCap = 'round';
  ctx.globalAlpha = 0.8;
  ctx.beginPath();
  ctx.moveTo(preview.ax, preview.ay);
  ctx.lineTo(preview.bx, preview.by);
  ctx.stroke();
  ctx.restore();
}

function drawJoints(ctx: CanvasRenderingContext2D, world: World, frame: RenderFrame): void {
  for (const node of world.nodes.values()) {
    const selected = frame.preview?.selectedId === node.id;
    const hover = frame.hoverNode === node.id;
    const r = node.anchored ? 11 : 8;
    ctx.beginPath();
    ctx.fillStyle = 'rgba(20,24,28,0.22)';
    ctx.arc(node.x + 1, node.y + 3, r + 2, 0, Math.PI * 2);
    ctx.fill();

    const g = ctx.createRadialGradient(node.x - 2, node.y - 2, 1, node.x, node.y, r + 2);
    if (node.anchored) {
      g.addColorStop(0, '#f0efe8');
      g.addColorStop(1, '#8a7a62');
    } else {
      g.addColorStop(0, '#f7f3ea');
      g.addColorStop(1, '#5c6570');
    }
    ctx.beginPath();
    ctx.fillStyle = g;
    ctx.arc(node.x, node.y, r, 0, Math.PI * 2);
    ctx.fill();
    ctx.strokeStyle = selected || hover ? '#f4d36a' : 'rgba(30,36,42,0.55)';
    ctx.lineWidth = selected ? 3 : 1.5;
    ctx.stroke();
  }
}

function drawSnap(ctx: CanvasRenderingContext2D, world: World): void {
  for (const event of world.snapEvents) {
    ctx.save();
    ctx.globalAlpha = 1 - event.age / 0.45;
    ctx.strokeStyle = '#ffd27a';
    ctx.lineWidth = 6;
    ctx.beginPath();
    ctx.moveTo(event.ax, event.ay);
    ctx.lineTo(event.bx, event.by);
    ctx.stroke();
    ctx.restore();
  }
}

function drawTrain(ctx: CanvasRenderingContext2D, train: TrainState, engine: EngineProfile, time: number): void {
  drawEngineAt(ctx, train.x, train.y, train.angle, engine, time, true);
}

function drawParkedEngine(ctx: CanvasRenderingContext2D, world: World, engine: EngineProfile): void {
  const start = world.node(world.startId);
  if (!start) {
    return;
  }
  drawEngineAt(ctx, start.x - 8, start.y - 10, 0, engine, 0, false);
}

function drawEngineAt(
  ctx: CanvasRenderingContext2D,
  x: number,
  y: number,
  angle: number,
  engine: EngineProfile,
  time: number,
  moving: boolean,
): void {
  ctx.save();
  ctx.translate(x, y);
  ctx.rotate(angle);
  ctx.fillStyle = 'rgba(20,24,30,0.2)';
  ctx.beginPath();
  ctx.ellipse(6, 16, 28, 6, 0, 0, Math.PI * 2);
  ctx.fill();

  roundedRect(ctx, -28, -16, 62, 22, 6);
  ctx.fillStyle = engine.color;
  ctx.fill();
  ctx.fillStyle = engine.cab;
  roundedRect(ctx, 12, -26, 22, 18, 4);
  ctx.fill();
  ctx.fillStyle = '#f4f1e6';
  ctx.fillRect(16, -22, 8, 7);
  ctx.fillStyle = '#2c333c';
  ctx.fillRect(-18, -28, 10, 12);
  ctx.fillRect(-8, -32, 6, 8);

  const spin = moving ? time * 8 : 0;
  drawWheel(ctx, -12, 10, 8, spin);
  drawWheel(ctx, 10, 10, 8, spin);
  if (engine.id === 'gordon' || engine.id === 'edward' || engine.id === 'james') {
    ctx.fillStyle = engine.color;
    roundedRect(ctx, -52, -12, 26, 16, 4);
    ctx.fill();
    drawWheel(ctx, -42, 10, 7, spin);
  }

  ctx.fillStyle = '#1b1f24';
  ctx.beginPath();
  ctx.arc(-6, -8, 3.2, 0, Math.PI * 2);
  ctx.fill();
  ctx.fillStyle = '#f2efe6';
  ctx.beginPath();
  ctx.arc(-7, -9, 1.3, 0, Math.PI * 2);
  ctx.fill();

  if (moving) {
    ctx.fillStyle = 'rgba(255,255,255,0.55)';
    ctx.beginPath();
    ctx.ellipse(-4, -40 - (time * 18) % 16, 7, 5, 0, 0, Math.PI * 2);
    ctx.fill();
  }
  ctx.restore();
}

function drawWheel(ctx: CanvasRenderingContext2D, x: number, y: number, r: number, spin: number): void {
  ctx.save();
  ctx.translate(x, y);
  ctx.rotate(spin);
  ctx.fillStyle = '#2a3036';
  ctx.beginPath();
  ctx.arc(0, 0, r, 0, Math.PI * 2);
  ctx.fill();
  ctx.strokeStyle = '#d8d2c4';
  ctx.lineWidth = 2;
  ctx.beginPath();
  ctx.arc(0, 0, r - 1.5, 0, Math.PI * 2);
  ctx.moveTo(-r + 2, 0);
  ctx.lineTo(r - 2, 0);
  ctx.stroke();
  ctx.restore();
}

function drawVignette(ctx: CanvasRenderingContext2D, w: number, h: number): void {
  const g = ctx.createRadialGradient(w / 2, h / 2, h * 0.2, w / 2, h / 2, h * 0.82);
  g.addColorStop(0, 'rgba(0,0,0,0)');
  g.addColorStop(1, 'rgba(20,28,36,0.18)');
  ctx.fillStyle = g;
  ctx.fillRect(0, 0, w, h);
}

function roundedRect(
  ctx: CanvasRenderingContext2D,
  x: number,
  y: number,
  w: number,
  h: number,
  r: number,
): void {
  const radius = Math.min(r, w / 2, h / 2);
  ctx.beginPath();
  ctx.moveTo(x + radius, y);
  ctx.arcTo(x + w, y, x + w, y + h, radius);
  ctx.arcTo(x + w, y + h, x, y + h, radius);
  ctx.arcTo(x, y + h, x, y, radius);
  ctx.arcTo(x, y, x + w, y, radius);
  ctx.closePath();
}

