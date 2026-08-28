import { test, expect } from '@playwright/test';
import {
  assertCanvasIsRendering,
  assertCanvasChanged,
  waitForRenderSettle,
} from './helpers';

test.describe('Gameplay smoke', () => {
  test('Start input advances past title screen', async ({ page }) => {
    await waitForRenderSettle(page, 3_000);
    await assertCanvasIsRendering(page);

    await page.evaluate(() => window.__TEST__!.tapButton('Start'));
    await assertCanvasChanged(page, 3_000);
  });

  test('title → level select → build mode flow via command queue', async ({ page }) => {
    await waitForRenderSettle(page, 3_000);

    await page.evaluate(() => window.__TEST__!.enqueue('start'));
    await page.waitForTimeout(1_500);
    await page.evaluate(() => window.__TEST__!.enqueue('start'));
    await page.waitForTimeout(1_500);
    await page.evaluate(() => window.__TEST__!.enqueue('start'));
    await page.waitForTimeout(2_000);

    const metrics = await assertCanvasIsRendering(page);
    expect(metrics.screenshotBytes).toBeGreaterThan(3_000);
  });

  test('theater mode keeps the canvas rendering', async ({ page }) => {
    await waitForRenderSettle(page);

    await page.locator('#btn-theater').click();
    await expect(page.locator('body')).toHaveClass(/theater-mode/);

    const metrics = await assertCanvasIsRendering(page, 1_500);
    expect(metrics.screenshotBytes).toBeGreaterThan(3_000);
  });
});
