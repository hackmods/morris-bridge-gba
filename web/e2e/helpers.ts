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

/** Shared held-input helpers for build macros. */
function createInputHelpers(page: Page) {
  const hold = async (buttons: string | string[], ms: number) => {
    if (Array.isArray(buttons)) {
      await page.evaluate(
        ({ list, duration }) => window.__TEST__!.holdButtons(list, duration),
        { list: buttons, duration: ms },
      );
    } else {
      await page.evaluate(
        ({ button, duration }) => window.__TEST__!.holdButton(button, duration),
        { button: buttons, duration: ms },
      );
    }
  };

  const tap = async (button: string) => {
    await page.evaluate((b) => window.__TEST__!.tapButton(b), button);
  };

  const fast = (direction: string, ms: number) => hold(['R', direction], ms);

  /** L+R+START loads host reference bridge then enters SIM (QA assist). */
  const runReference = async () => {
    await page.evaluate(() => window.__TEST__!.holdButtons(['L', 'R', 'Start'], 280));
    await page.waitForTimeout(400);
  };

  return { hold, tap, fast, runReference };
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
  await tap('Down');
  await tap('Down');
  await page.waitForTimeout(400);
  await tap('Start');
  await page.waitForTimeout(1_500);
}

/** Build the Knapford reference bridge (3 deck spans) via held D-pad input. */
export async function buildKnapfordReferenceBridge(page: Page): Promise<void> {
  const { tap, fast } = createInputHelpers(page);
  await fast('Left', 220);
  await tap('A');
  await fast('Right', 420);
  await tap('A');
  await fast('Left', 420);
  await tap('A');
  await fast('Right', 820);
  await tap('A');
  await fast('Left', 420);
  await tap('A');
  await fast('Right', 420);
  await tap('A');
  await fast('Right', 420);
  await tap('A');

  await page.waitForTimeout(500);
}

/** Single START→END span — snaps under Thomas (host M2 simulation). */
export async function buildOverlongKnapfordSpan(page: Page): Promise<void> {
  const { tap, fast } = createInputHelpers(page);
  await fast('Left', 220);
  await tap('A');
  await fast('Right', 1_200);
  await tap('A');
  await page.waitForTimeout(500);
}

/** Build Gordon's Hill reference track + braces (host playthrough layout). */
export async function buildGordonHillReferenceBridge(page: Page): Promise<void> {
  const { tap, fast, hold } = createInputHelpers(page);

  // Cursor boots near START (-80, 44) on raised deck.
  await fast('Left', 220);
  await tap('A');
  await fast('Right', 420);
  await tap('A');
  await fast('Left', 420);
  await tap('A');
  await fast('Right', 420);
  await tap('A');
  await fast('Left', 420);
  await tap('A');
  await fast('Right', 420);
  await tap('A');
  await fast('Left', 420);
  await tap('A');
  await fast('Right', 420);
  await tap('A');
  await fast('Left', 420);
  await tap('A');
  await fast('Right', 420);
  await tap('A');
  await fast('Right', 420);
  await tap('A');

  await tap('R');
  await fast('Left', 220);
  await tap('A');
  await hold(['R', 'Down'], 280);
  await hold(['R', 'Right'], 220);
  await tap('A');

  await fast('Right', 1_400);
  await tap('A');
  await hold(['R', 'Down'], 280);
  await hold(['R', 'Left'], 220);
  await tap('A');

  await page.waitForTimeout(500);
}

/** Vicarstown build with ?walkthrough=vicarstown (all_unlocked save). */
export async function navigateToVicarstownBuildMode(page: Page): Promise<void> {
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
  await tap('Down');
  await tap('Down');
  await tap('Down');
  await page.waitForTimeout(400);
  await tap('Start');
  await page.waitForTimeout(1_500);
}

/** Build Vicarstown reference track + START brace (host playthrough layout). */
export async function buildVicarstownReferenceBridge(page: Page): Promise<void> {
  const { tap, fast, hold } = createInputHelpers(page);

  await fast('Left', 220);
  await tap('A');
  await fast('Right', 420);
  await tap('A');
  await fast('Left', 420);
  await tap('A');
  await fast('Right', 420);
  await tap('A');
  await fast('Left', 420);
  await tap('A');
  await fast('Right', 420);
  await tap('A');
  await fast('Left', 420);
  await tap('A');
  await fast('Right', 420);
  await tap('A');
  await fast('Left', 420);
  await tap('A');
  await fast('Right', 420);
  await tap('A');
  await fast('Left', 420);
  await tap('A');
  await fast('Right', 420);
  await tap('A');
  await fast('Left', 420);
  await tap('A');
  await fast('Right', 420);
  await tap('A');
  await fast('Right', 420);
  await tap('A');

  await tap('R');
  await fast('Left', 220);
  await tap('A');
  await hold(['R', 'Down'], 280);
  await hold(['R', 'Right'], 220);
  await tap('A');

  await page.waitForTimeout(500);
}
