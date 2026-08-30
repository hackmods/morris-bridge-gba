import { ENGINES } from './game/engines';
import { BridgeGame, formatCost, type ScreenId } from './game/game';
import { LEVELS } from './game/levels';
import { MATERIALS } from './game/materials';
import { isEngineUnlocked, isLevelUnlocked } from './game/progress';

function $(id: string): HTMLElement {
  const el = document.getElementById(id);
  if (!el) {
    throw new Error(`Missing #${id}`);
  }
  return el;
}

const canvas = $('world') as HTMLCanvasElement;
const game = new BridgeGame(canvas);
const hooks = game.hooks();

$('btn-play').addEventListener('click', () => {
  game.goLevels();
});
$('btn-levels-back').addEventListener('click', () => game.goTitle());
$('btn-engines-back').addEventListener('click', () => game.goLevels());
$('btn-engines-go').addEventListener('click', () => game.goBuild());
$('btn-menu').addEventListener('click', () => game.goLevels());
$('btn-undo').addEventListener('click', () => game.world.undo());
$('btn-test').addEventListener('click', () => {
  game.startTest();
});
$('btn-cancel').addEventListener('click', () => game.cancelTest());
$('btn-retry').addEventListener('click', () => game.goBuild());
$('btn-next').addEventListener('click', () => {
  const next = Math.min(LEVELS.length - 1, game.levelIndex + 1);
  game.goEngines(next);
});

function showScreen(id: ScreenId): void {
  for (const screen of ['title', 'levels', 'engines', 'result'] as const) {
    const el = $(`screen-${screen}`);
    el.classList.toggle('hidden', id !== screen);
  }
  $('hud').classList.toggle('hidden', id !== 'build');
}

function renderLevels(): void {
  const grid = $('level-grid');
  grid.innerHTML = '';
  LEVELS.forEach((level, index) => {
    const unlocked = isLevelUnlocked(game.progress, index);
    const card = document.createElement('button');
    card.type = 'button';
    card.className = 'card';
    card.disabled = !unlocked;
    const stars = game.progress.stars[index] ?? 0;
    card.innerHTML = `
      <strong>${index + 1}. ${level.name}</strong>
      <span>${level.region}</span>
      <span class="stars">${'★'.repeat(stars)}${'☆'.repeat(3 - stars)}</span>
    `;
    card.addEventListener('click', () => game.goEngines(index));
    grid.append(card);
  });
}

function renderEngines(): void {
  $('engine-level-name').textContent = game.level.name;
  const grid = $('engine-grid');
  grid.innerHTML = '';
  for (const engine of ENGINES) {
    const unlocked = isEngineUnlocked(game.progress, engine.id);
    const card = document.createElement('button');
    card.type = 'button';
    card.className = `card ${game.engineId === engine.id ? 'active' : ''}`;
    card.disabled = !unlocked;
    card.innerHTML = `
      <div class="engine-swatch" style="background:${engine.color}"></div>
      <strong>${engine.name}</strong>
      <span>${engine.title}</span>
      <span>Load ${engine.weight} · ${engine.blurb}</span>
    `;
    card.addEventListener('click', () => {
      game.selectEngine(engine.id);
    });
    grid.append(card);
  }
}

function renderTools(): void {
  const tools = $('tools');
  tools.innerHTML = '';
  const ids = [...game.level.tools, 'erase'] as const;
  for (const id of ids) {
    const button = document.createElement('button');
    button.type = 'button';
    button.className = `tool ${id} ${game.tool === id ? 'active' : ''}`;
    if (id === 'erase') {
      button.innerHTML = `<strong>Erase</strong><small>Click a span</small>`;
    } else {
      const mat = MATERIALS[id];
      button.innerHTML = `<strong>${mat.name}</strong><small>${mat.hint}</small>`;
    }
    button.addEventListener('click', () => game.selectTool(id));
    tools.append(button);
  }
}

function tutorialText(): string {
  if (game.progress.seenTutorial || game.levelIndex !== 0) {
    return '';
  }
  if (game.tutorialStep <= 0) {
    return 'Click a bank joint, then drag to weld a road toward the far side.';
  }
  if (game.tutorialStep === 1) {
    return 'Release on the opposite bank or in mid-air to plant the next joint.';
  }
  if (game.tutorialStep === 2) {
    return 'Keep welding road until START and END are linked. Timber braces stop sag.';
  }
  return 'Press Test crossing when the road is complete.';
}

function renderHud(rebuildTools: boolean): void {
  $('hud-region').textContent = game.level.region;
  $('hud-title').textContent = game.level.name;
  $('hud-cost').textContent = formatCost(game.world.totalCost());
  $('hud-budget').textContent = formatCost(game.level.budget);
  $('hud-par').textContent = formatCost(game.level.parCost);
  $('hud-cost').style.color = game.world.totalCost() > game.level.budget ? 'var(--bad)' : '';
  const testing = game.world.status === 'running';
  $('btn-test').classList.toggle('hidden', testing);
  $('btn-cancel').classList.toggle('hidden', !testing);
  const text = tutorialText();
  $('tutorial').textContent = text;
  $('tutorial').classList.toggle('hidden', text.length === 0);
  if (game.toast) {
    $('toast').textContent = game.toast;
    $('toast').classList.remove('hidden');
  } else {
    $('toast').classList.add('hidden');
  }
  if (rebuildTools) {
    renderTools();
  }
}

function renderResult(): void {
  const win = game.lastStatus === 'success';
  $('result-eyebrow').textContent = win ? 'Clear' : 'Collapse';
  $('result-title').textContent = win ? 'The train is through' : 'The crossing failed';
  $('result-stars').textContent = win ? '★'.repeat(game.lastStars) + '☆'.repeat(3 - game.lastStars) : '☆☆☆';
  $('result-copy').textContent = win
    ? `${formatCost(game.world.totalCost())} against a par of ${formatCost(game.level.parCost)}.`
    : 'A span tore, or the road never reached the far bank. Rebuild and weld shorter, stronger members.';
  $('btn-next').classList.toggle('hidden', !win || game.levelIndex >= LEVELS.length - 1);
}

window.__GAME__ = hooks;

let lastScreen = '';
let lastEngine = '';
let lastTool = '';

function tick(): void {
  showScreen(game.screen);
  if (game.screen === 'levels' && lastScreen !== 'levels') {
    renderLevels();
  }
  if (game.screen === 'engines' && (lastScreen !== 'engines' || lastEngine !== game.engineId)) {
    renderEngines();
  }
  if (game.screen === 'build') {
    renderHud(lastTool !== game.tool || lastScreen !== 'build');
  }
  if (game.screen === 'result' && lastScreen !== 'result') {
    renderResult();
  }
  lastScreen = game.screen;
  lastEngine = game.engineId;
  lastTool = game.tool;
  requestAnimationFrame(tick);
}

game.start();
tick();
