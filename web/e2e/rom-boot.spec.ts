import { test, expect } from '@playwright/test';
import { assertCanvasIsRendering, measureCanvasRender, waitForPlaytestReady, waitForRenderSettle } from './helpers';

test.describe('ROM boot', () => {
  test('loads thomas_bridge.gba without error banner', async ({ page }) => {
    await waitForPlaytestReady(page);

    await expect(page.locator('#error-banner')).toHaveClass(/hidden/);
    await expect(page.locator('#btn-reset')).toBeEnabled();
    await expect(page.locator('#command-input')).toBeEnabled();
  });

  test('reports cross-origin isolation (required for mGBA threads)', async ({ page }) => {
    await page.goto('/legacy.html');

    const isolated = await page.evaluate(() => crossOriginIsolated);
    expect(isolated).toBe(true);
  });

  test('emulator produces a rendered canvas after boot', async ({ page }) => {
    await waitForRenderSettle(page);
    const metrics = await assertCanvasIsRendering(page);

    expect(metrics.screenshotBytes).toBeGreaterThan(3_000);
  });
});
