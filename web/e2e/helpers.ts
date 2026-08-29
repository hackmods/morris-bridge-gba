import { expect, type Page } from '@playwright/test';

export async function waitForPlaytestReady(page: Page): Promise<void> {
  await page.goto('/');

  await expect(page.locator('#error-banner')).toHaveClass(/hidden/);
  await expect(page.locator('#status-bar')).toHaveClass(/ready/, { timeout: 60_000 });
  await expect(page.locator('#status-bar')).toContainText('thomas_bridge.gba');

  await page.waitForFunction(() => Boolean(window.__TEST__?.isReady()));
}

export async function waitForRenderSettle(page: Page, ms = 2_500): Promise<void> {
  await waitForPlaytestReady(page);
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
export async function navigateToBuildMode(page: Page): Promise<void> {
  await waitForRenderSettle(page, 3_000);

  for (let step = 0; step < 4; ++step) {
    await page.evaluate(() => window.__TEST__!.tapButton('Start'));
    await page.waitForTimeout(1_200);
  }

  await page.waitForTimeout(1_500);
}
