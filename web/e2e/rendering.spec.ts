import { test, expect } from '@playwright/test';
import { assertCanvasIsRendering, waitForPlaytestReady, waitForRenderSettle } from './helpers';

test.describe('Rendering', () => {
  test('canvas draws visible game pixels (not a blank framebuffer)', async ({ page }) => {
    await waitForRenderSettle(page);
    const metrics = await assertCanvasIsRendering(page);

    expect(metrics.screenshotBytes).toBeGreaterThan(3_500);
  });

  test('continues rendering after soft reset', async ({ page }) => {
    await waitForRenderSettle(page);

    await page.locator('#btn-reset').click();
    await page.waitForTimeout(2_000);

    const metrics = await assertCanvasIsRendering(page);
    expect(metrics.screenshotBytes).toBeGreaterThan(3_000);
  });

  test('canvas screenshot is non-trivial PNG payload', async ({ page }) => {
    await waitForRenderSettle(page);

    const screenshot = await page.locator('#game-canvas').screenshot();
    expect(screenshot.byteLength).toBeGreaterThan(3_000);
    expect(new Set(screenshot).size).toBeGreaterThan(32);
  });
});
