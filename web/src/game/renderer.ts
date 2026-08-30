import { type EngineProfile } from './engines';
import type { LevelDef } from './levels';
import { MATERIALS, spanHeat, type MaterialId } from './materials';
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
  cost: number;
  selectedId: number | null;
}

export interface RenderFrame {
  world: World;
  level: LevelDef;
  camera: Camera;
  time: number;
  engine: EngineProfile;
  load: number;
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
  if (!frame.testing) {
    drawGrid(ctx, level);
  }
  drawWater(ctx, level, time);
  drawTerrain(ctx, level);
  drawFoliage(ctx, level);
  if (!frame.testing) {
    drawGuide(ctx, world);
  }
  drawBeams(ctx, world, frame);
  if (frame.preview) {
    drawPreview(ctx, frame.preview, frame.load);
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
  g.addColorStop(0.42, level.theme.skyHorizon);
  g.addColorStop(0.72, '#f4e4c4');
  g.addColorStop(1, '#f6efe2');
  ctx.fillStyle = g;
  ctx.fillRect(0, 0, w, h);

  const sunX = w * 0.78;
  const sunY = h * 0.16 + Math.sin(time * 0.15) * 4;
  const halo = ctx.createRadialGradient(sunX, sunY, 6, sunX, sunY, 180);
  halo.addColorStop(0, '#fff6d0');
  halo.addColorStop(0.18, level.theme.sun);
  halo.addColorStop(1, 'rgba(255, 230, 180, 0)');
  ctx.fillStyle = halo;
  ctx.beginPath();
  ctx.arc(sunX, sunY, 180, 0, Math.PI * 2);
  ctx.fill();

  ctx.fillStyle = 'rgba(255,255,255,0.62)';
  drawCloud(ctx, w * 0.16, h * 0.12, 52, time * 5);
  ctx.fillStyle = 'rgba(255,255,255,0.48)';
  drawCloud(ctx, w * 0.52, h * 0.09, 40, time * 3.4);
  ctx.fillStyle = 'rgba(255,255,255,0.36)';
  drawCloud(ctx, w * 0.86, h * 0.2, 28, time * 2.2);
}

function drawCloud(ctx: CanvasRenderingContext2D, x: number, y: number, r: number, drift: number): void {
  const dx = (drift % 900) - 60;
  ctx.beginPath();
  ctx.ellipse(x + dx, y, r * 1.7, r * 0.52, 0, 0, Math.PI * 2);
  ctx.ellipse(x + dx + r * 0.75, y - 10, r * 1.05, r * 0.48, 0, 0, Math.PI * 2);
  ctx.ellipse(x + dx - r * 0.7, y - 6, r * 0.95, r * 0.42, 0, 0, Math.PI * 2);
  ctx.ellipse(x + dx + r * 0.15, y - 16, r * 0.7, r * 0.36, 0, 0, Math.PI * 2);
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

  ctx.fillStyle = 'rgba(40, 60, 40, 0.18)';
  for (let x = 40; x < w; x += 70) {
    const base = h * 0.52 + Math.sin(x * 0.006 + 0.4) * 28;
    ctx.beginPath();
    ctx.moveTo(x, base);
    ctx.lineTo(x + 8, base - 22);
    ctx.lineTo(x + 16, base);
    ctx.fill();
  }

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

function drawGrid(ctx: CanvasRenderingContext2D, level: LevelDef): void {
  ctx.save();
  ctx.strokeStyle = 'rgba(255,255,255,0.14)';
  ctx.lineWidth = 1;
  for (let x = 0; x <= level.width; x += 40) {
    ctx.beginPath();
    ctx.moveTo(x, 0);
    ctx.lineTo(x, level.height);
    ctx.stroke();
  }
  for (let y = 0; y <= level.height; y += 40) {
    ctx.beginPath();
    ctx.moveTo(-40, y);
    ctx.lineTo(level.width + 40, y);
    ctx.stroke();
  }
  ctx.restore();
}

function drawWater(ctx: CanvasRenderingContext2D, level: LevelDef, time: number): void {
  const y = level.waterY;
  const w = level.width;
  const g = ctx.createLinearGradient(0, y - 30, 0, level.height);
  g.addColorStop(0, level.theme.water);
  g.addColorStop(0.45, mixColor(level.theme.water, level.theme.waterDeep, 0.45));
  g.addColorStop(1, level.theme.waterDeep);
  ctx.fillStyle = g;
  ctx.fillRect(-100, y - 8, w + 200, level.height - y + 40);

  ctx.strokeStyle = 'rgba(255,255,255,0.32)';
  ctx.lineWidth = 2;
  for (let i = 0; i < 6; i += 1) {
    ctx.beginPath();
    const yy = y + 8 + i * 13;
    for (let x = 0; x <= w; x += 14) {
      const wave = Math.sin(x * 0.02 + time * 1.4 + i) * 3.2;
      if (x === 0) {
        ctx.moveTo(x, yy + wave);
      } else {
        ctx.lineTo(x, yy + wave);
      }
    }
    ctx.stroke();
  }

  ctx.fillStyle = 'rgba(255,255,255,0.08)';
  ctx.fillRect(-100, y - 10, w + 200, 10);
}

function drawTerrain(ctx: CanvasRenderingContext2D, level: LevelDef): void {
  for (const pad of level.terrain) {
    if (pad.y > level.waterY + 30) {
      continue;
    }
    const width = pad.x1 - pad.x0;
    ctx.fillStyle = level.theme.dirtDark;
    ctx.fillRect(pad.x0, pad.y, width, level.height - pad.y + 40);

    const face = ctx.createLinearGradient(pad.x0, pad.y, pad.x0, pad.y + 90);
    face.addColorStop(0, level.theme.dirt);
    face.addColorStop(1, level.theme.dirtDark);
    ctx.fillStyle = face;
    ctx.beginPath();
    ctx.moveTo(pad.x0, pad.y + 22);
    ctx.lineTo(pad.x0, pad.y);
    ctx.lineTo(pad.x1, pad.y);
    ctx.lineTo(pad.x1, pad.y + 22);
    ctx.closePath();
    ctx.fill();

    ctx.fillStyle = level.theme.grass;
    ctx.beginPath();
    ctx.moveTo(pad.x0 - 6, pad.y + 4);
    ctx.quadraticCurveTo(pad.x0 + width / 2, pad.y - 16, pad.x1 + 6, pad.y + 4);
    ctx.lineTo(pad.x1, pad.y + 8);
    ctx.lineTo(pad.x0, pad.y + 8);
    ctx.closePath();
    ctx.fill();

    ctx.fillStyle = level.theme.grassDark;
    for (let x = pad.x0 + 8; x < pad.x1; x += 16) {
      ctx.beginPath();
      ctx.moveTo(x, pad.y - 4);
      ctx.lineTo(x + 5, pad.y - 18 - ((x * 13) % 7));
      ctx.lineTo(x + 10, pad.y - 4);
      ctx.fill();
    }

    drawAbutment(ctx, pad.x1 - 18, pad.y, true, level);
    drawAbutment(ctx, pad.x0, pad.y, false, level);
  }
}

function drawAbutment(
  ctx: CanvasRenderingContext2D,
  x: number,
  y: number,
  leftBank: boolean,
  level: LevelDef,
): void {
  const dir = leftBank ? 1 : -1;
  ctx.fillStyle = '#8d8374';
  ctx.fillRect(x, y, 18, 54);
  ctx.fillStyle = '#6f6756';
  ctx.fillRect(x + (leftBank ? 14 : 0), y, 4, 54);
  ctx.fillStyle = '#c2b49a';
  for (let row = 0; row < 4; row += 1) {
    ctx.fillRect(x + 2 + (row % 2) * 3, y + 8 + row * 11, 12, 3);
  }
  ctx.fillStyle = level.theme.dirtDark;
  ctx.beginPath();
  ctx.moveTo(x + (leftBank ? 18 : 0), y + 54);
  ctx.lineTo(x + (leftBank ? 18 : 0) + dir * 22, y + 86);
  ctx.lineTo(x + (leftBank ? 18 : 0), y + 86);
  ctx.closePath();
  ctx.fill();
}

function drawFoliage(ctx: CanvasRenderingContext2D, level: LevelDef): void {
  for (const pad of level.terrain) {
    if (pad.y > level.waterY + 30) {
      continue;
    }
    for (let x = pad.x0 + 28; x < pad.x1 - 36; x += 54) {
      const seed = ((x * 17) | 0) % 5;
      if (seed === 0) {
        continue;
      }
      drawTree(ctx, x + seed * 3, pad.y - 8, 16 + seed * 2, level.theme.grassDark);
    }
  }
}

function drawTree(
  ctx: CanvasRenderingContext2D,
  x: number,
  y: number,
  r: number,
  leaf: string,
): void {
  ctx.fillStyle = '#5a4630';
  ctx.fillRect(x - 3, y - r * 0.2, 6, r * 0.7);
  ctx.fillStyle = leaf;
  ctx.beginPath();
  ctx.ellipse(x, y - r * 0.7, r * 0.85, r * 0.7, 0, 0, Math.PI * 2);
  ctx.ellipse(x - r * 0.45, y - r * 0.4, r * 0.55, r * 0.45, 0, 0, Math.PI * 2);
  ctx.ellipse(x + r * 0.4, y - r * 0.45, r * 0.5, r * 0.42, 0, 0, Math.PI * 2);
  ctx.fill();
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
    drawBeam(ctx, a, b, beam, frame.testing || world.status === 'running', hover, frame.load);
  }
}

function drawBeam(
  ctx: CanvasRenderingContext2D,
  a: Node,
  b: Node,
  beam: Beam,
  showStress: boolean,
  hover: boolean,
  load: number,
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

  const heat = showStress
    ? clamp(beam.stress * 1.15, 0, 1)
    : spanHeat(length, beam.material, load);
  const fill = heat < 0.58 ? mat.color : mixColor(mat.color, '#e23b2f', clamp((heat - 0.5) / 0.5, 0, 1));
  ctx.fillStyle = fill;
  roundedRect(ctx, 0, -width / 2, length, width, 4);
  ctx.fill();

  if (beam.material === 'road') {
    ctx.fillStyle = heat >= 0.72 ? '#f0c27a' : mat.colorLight;
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

function drawPreview(ctx: CanvasRenderingContext2D, preview: DrawPreview, load: number): void {
  if (preview.ax === 0 && preview.bx === 0 && preview.ay === 0 && preview.by === 0) {
    return;
  }
  const mat = MATERIALS[preview.material];
  const length = Math.hypot(preview.bx - preview.ax, preview.by - preview.ay);
  const heat = spanHeat(length, preview.material, load);
  ctx.save();
  ctx.setLineDash(preview.ok ? [] : [8, 6]);
  ctx.strokeStyle = preview.weak || !preview.ok || heat >= 0.72 ? '#e35b4a' : mixColor(mat.colorLight, '#e23b2f', clamp((heat - 0.5) / 0.5, 0, 1));
  ctx.lineWidth = preview.material === 'road' ? 10 : 7;
  ctx.lineCap = 'round';
  ctx.globalAlpha = 0.84;
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
  ctx.fillStyle = 'rgba(20,24,30,0.22)';
  ctx.beginPath();
  ctx.ellipse(8, 18, 34, 7, 0, 0, Math.PI * 2);
  ctx.fill();

  if (engine.id === 'gordon' || engine.id === 'edward' || engine.id === 'james') {
    ctx.fillStyle = mixColor(engine.color, '#1a2430', 0.18);
    roundedRect(ctx, -58, -14, 28, 18, 5);
    ctx.fill();
    ctx.fillStyle = '#d8c9a4';
    ctx.fillRect(-56, -8, 24, 4);
    drawWheel(ctx, -46, 10, 7, moving ? time * 8 : 0);
  }

  roundedRect(ctx, -30, -18, 68, 24, 7);
  ctx.fillStyle = engine.color;
  ctx.fill();
  ctx.fillStyle = mixColor(engine.color, '#ffffff', 0.18);
  roundedRect(ctx, -26, -16, 58, 8, 4);
  ctx.fill();

  ctx.fillStyle = engine.cab;
  roundedRect(ctx, 14, -30, 24, 20, 5);
  ctx.fill();
  ctx.fillStyle = '#f4f1e6';
  ctx.fillRect(18, -26, 8, 8);
  ctx.fillStyle = 'rgba(40,70,110,0.35)';
  ctx.fillRect(18, -26, 8, 8);

  ctx.fillStyle = '#2c333c';
  ctx.fillRect(-16, -30, 12, 14);
  ctx.fillRect(-6, -36, 7, 10);
  ctx.fillStyle = '#c45c4a';
  ctx.beginPath();
  ctx.arc(-2, -38, 5, 0, Math.PI * 2);
  ctx.fill();

  ctx.fillStyle = '#f3e6c8';
  ctx.beginPath();
  ctx.arc(-22, -6, 9, 0, Math.PI * 2);
  ctx.fill();
  ctx.strokeStyle = 'rgba(40,36,30,0.45)';
  ctx.lineWidth = 1.2;
  ctx.stroke();
  ctx.fillStyle = '#1b1f24';
  ctx.beginPath();
  ctx.arc(-25, -8, 1.6, 0, Math.PI * 2);
  ctx.arc(-19, -8, 1.6, 0, Math.PI * 2);
  ctx.fill();
  ctx.strokeStyle = '#1b1f24';
  ctx.beginPath();
  ctx.arc(-22, -3, 3.2, 0.15, Math.PI - 0.15);
  ctx.stroke();

  ctx.fillStyle = '#d8c9a4';
  ctx.fillRect(-30, 2, 66, 4);
  ctx.fillStyle = '#8a6a3c';
  ctx.fillRect(34, 0, 8, 5);

  const spin = moving ? time * 8 : 0;
  drawWheel(ctx, -12, 10, 8, spin);
  drawWheel(ctx, 12, 10, 8, spin);

  if (moving) {
    ctx.fillStyle = 'rgba(255,255,255,0.55)';
    ctx.beginPath();
    ctx.ellipse(-4, -46 - (time * 18) % 16, 8, 6, 0, 0, Math.PI * 2);
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
