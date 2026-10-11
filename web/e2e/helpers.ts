import { expect, type Page } from '@playwright/test';

export async function waitForPlaytestReady(page: Page, url?: string): Promise<void> {
  if (url !== undefined) {
    await page.goto(url);
  } else if (new URL(page.url()).search === '') {
    await page.goto('/');
  }

  await expect(page.locator('#error-banner')).toHaveClass(/hidden/);
  await expect(page.locator('#status-bar')).toHaveClass(/ready/, { timeout: 60_000 });
  await expect(page.locator('#status-bar')).toContainText('thomas_bridge.gba');

  await page.waitForFunction(() => Boolean(window.__TEST__?.isReady()));
}

export async function waitForRenderSettle(page: Page, ms = 2_500, url?: string): Promise<void> {
  await waitForPlaytestReady(page, url);
  await page.waitForTimeout(ms);
}

export interface CanvasRenderMetrics {
  screenshotBytes: number;
  width: number;
  height: number;
}

export async function captureCanvasScreenshot(page: Page): Promise<Buffer> {
  return page.locator('#game-canvas').screenshot();
}

export async function measureCanvasRender(page: Page): Promise<CanvasRenderMetrics> {
  const screenshot = await captureCanvasScreenshot(page);
  const size = await page.evaluate(() => {
    const canvas = document.getElementById('game-canvas') as HTMLCanvasElement;
    return { width: canvas.width, height: canvas.height };
  });

  return {
    screenshotBytes: screenshot.byteLength,
    width: size.width,
    height: size.height,
  };
}

export async function assertCanvasIsRendering(
  page: Page,
  settleMs = 500,
): Promise<CanvasRenderMetrics> {
  await page.waitForTimeout(settleMs);

  const metrics = await measureCanvasRender(page);

  expect(metrics.width).toBe(240);
  expect(metrics.height).toBe(160);
  expect(metrics.screenshotBytes).toBeGreaterThan(3_000);

  return metrics;
}

export async function assertEmulatorStillRunning(page: Page): Promise<void> {
  const before = await captureCanvasScreenshot(page);
  expect(before.byteLength).toBeGreaterThan(3_000);

  await page.waitForTimeout(2_000);

  const after = await captureCanvasScreenshot(page);
  expect(after.byteLength).toBeGreaterThan(3_000);
}

export async function assertCanvasChanged(page: Page, waitMs = 2_500): Promise<void> {
  const before = await captureCanvasScreenshot(page);

  await page.waitForTimeout(waitMs);

  const after = await captureCanvasScreenshot(page);
  expect(after.equals(before)).toBe(false);
}

export async function screenshotCanvas(page: Page, name: string): Promise<void> {
  await page.locator('#game-canvas').screenshot({ path: `test-results/${name}.png` });
}

/** Title → level select → engine select → (intro) → build mode. */
export async function navigateToLevelSelect(page: Page, url?: string): Promise<void> {
  await waitForRenderSettle(page, 3_000, url);
  await page.evaluate(() => window.__TEST__!.tapButton('Start'));
  await page.waitForTimeout(1_500);
}

/** Title → level select → engine select → (intro) → build mode. */
export async function navigateToBuildMode(page: Page): Promise<void> {
  await waitForRenderSettle(page, 3_000);

  for (let step = 0; step < 4; ++step) {
    await page.evaluate(() => window.__TEST__!.tapButton('Start'));
    await page.waitForTimeout(1_200);
  }

  await page.waitForTimeout(1_500);
}

/**
 * Build-cursor helpers. The cursor moves 3 px per frame while a direction is
 * held, so a move is a timed hold; joints snap within 10 px and new joints
 * land on the 4 px grid, which absorbs a frame or two of timing error.
 *
 * The layouts below are the par designs from include/level_solutions.h.
 */
function createInputHelpers(page: Page) {
  const tap = async (button: string, settleMs = 280) => {
    await page.evaluate((b) => window.__TEST__!.tapButton(b), button);
    await page.waitForTimeout(settleMs);
  };

  const move = async (direction: string, pixels: number) => {
    const ms = Math.round((pixels / 3) * (1000 / 60));
    await page.evaluate(
      ({ button, duration }) => window.__TEST__!.holdButton(button, duration),
      { button: direction, duration: ms },
    );
    await page.waitForTimeout(180);
  };

  return { tap, move };
}

/** Gordon's Hill build with ?walkthrough=gordon save (intro skipped). */
export async function navigateToGordonBuildMode(page: Page): Promise<void> {
  await page.goto('/?walkthrough=gordon');
  await waitForRenderSettle(page, 3_000);

  const { tap } = createInputHelpers(page);

  await tap('Start');
  await page.waitForTimeout(1_200);
  await tap('Down');
  await page.waitForTimeout(800);
  await tap('Start');
  await page.waitForTimeout(1_200);
  // Thomas is timetabled for the hill; Edward banks from behind.
  await page.waitForTimeout(400);
  await tap('Start');
  await page.waitForTimeout(1_500);
}

/** Knapford deck only: START - joint - END with nothing under it. Collapses. */
export async function buildUnsupportedKnapfordDeck(page: Page): Promise<void> {
  const { tap, move } = createInputHelpers(page);

  // Cursor boots 16 px right of START (-40, 12).
  await move('Left', 16);
  await tap('A');
  await move('Right', 40);
  await tap('A');
  await move('Right', 40);
  await tap('A');
  await tap('B');

  await page.waitForTimeout(300);
}

/** Knapford par design: the deck joint propped by a timber post on the rock. */
export async function buildKnapfordReferenceBridge(page: Page): Promise<void> {
  const { tap, move } = createInputHelpers(page);

  await buildUnsupportedKnapfordDeck(page);

  // Back to the mid joint, then weld straight down to the rock at (0, 40).
  await move('Left', 40);
  await tap('A');
  await move('Down', 28);
  await tap('A');
  await tap('B');

  await page.waitForTimeout(300);
}

/** Gordon's Hill par design: the deck climbs 32 px, each joint hung from a crag on a rod. */
export async function buildGordonHillReferenceBridge(page: Page): Promise<void> {
  const { tap, move } = createInputHelpers(page);

  // Cursor boots 16 px right of START (-80, 44). Joints at (-40, 36), (0, 28), (40, 20).
  await move('Left', 16);
  await tap('A');

  for (let span = 0; span < 4; ++span) {
    await move('Right', 40);
    await move('Up', 8);
    await tap('A');
  }

  await tap('B');
  await tap('R');

  // Right joint (40, 20) up to the right crag (36, -36).
  await move('Left', 40);
  await move('Down', 8);
  await tap('A');
  await move('Up', 56);
  await tap('A');
  await tap('B');

  // Left crag (-36, -24) down to the left joint (-40, 36).
  await move('Left', 76);
  await move('Down', 12);
  await tap('A');
  await move('Down', 60);
  await tap('A');
  await tap('B');

  // Left crag again, across and down to the middle joint (0, 28).
  await move('Up', 60);
  await tap('A');
  await move('Right', 36);
  await move('Down', 52);
  await tap('A');
  await tap('B');

  await page.waitForTimeout(300);
}

/** Culdee Fell (track 3) build with the all_unlocked save (?walkthrough=vicarstown). */
export async function navigateToCuldeeFellBuildMode(page: Page): Promise<void> {
  await page.goto('/?walkthrough=vicarstown');
  await waitForRenderSettle(page, 3_000);

  const { tap } = createInputHelpers(page);

  await tap('Start');
  await page.waitForTimeout(1_200);
  await tap('Down');
  await tap('Down');
  await page.waitForTimeout(800);
  await tap('Start');
  await page.waitForTimeout(1_200);
  // Engine menu runs No. 1 to No. 6; Culdee Fell is timetabled for Edward, No. 2.
  await tap('Down');
  await page.waitForTimeout(400);
  await tap('Start');
  await page.waitForTimeout(1_500);
}

/** Culdee Fell par design: Warren truss, timber inside, rods for end posts and top chord. */
export async function buildCuldeeFellReferenceBridge(page: Page): Promise<void> {
  const { tap, move } = createInputHelpers(page);

  // Cursor boots 16 px right of START (-60, 30). Deck joints at -20 and 20.
  await move('Left', 16);
  await tap('A');

  for (let span = 0; span < 3; ++span) {
    await move('Right', 40);
    await tap('A');
  }

  await tap('B');

  // Timber diagonals, right to left; each weld plants the next top joint.
  await move('Left', 40);
  await tap('A');
  await move('Up', 30);
  await move('Right', 20);
  await tap('A');
  await tap('B');

  await move('Left', 20);
  await move('Down', 30);
  await tap('A');
  await move('Up', 30);
  await move('Left', 20);
  await tap('A');
  await move('Left', 20);
  await move('Down', 30);
  await tap('A');
  await move('Up', 30);
  await move('Left', 20);
  await tap('A');
  await tap('B');

  // Rods: left end post, the top chord, right end post.
  await tap('R');
  await tap('A');
  await move('Left', 20);
  await move('Down', 30);
  await tap('A');
  await tap('B');

  await move('Up', 30);
  await move('Right', 20);
  await tap('A');
  await move('Right', 40);
  await tap('A');
  await move('Right', 40);
  await tap('A');
  await move('Right', 20);
  await move('Down', 30);
  await tap('A');
  await tap('B');

  await page.waitForTimeout(300);
}
