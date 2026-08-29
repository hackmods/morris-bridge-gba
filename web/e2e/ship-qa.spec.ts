import { test, expect } from '@playwright/test';
import {
  assertCanvasChanged,
  assertCanvasIsRendering,
  navigateToBuildMode,
  waitForRenderSettle,
} from './helpers';

/**
 * Master-plan M1–M5 screen-flow checks via mGBA-WASM web playtest.
 * M1/M2 simulation wins are covered by host playthrough tests (see docs/ship-qa.md).
 */
test.describe('Ship QA screen flows (M3–M5)', () => {
  test('M3: empty bridge returns to build after NO GO', async ({ page }) => {
    await navigateToBuildMode(page);
    await assertCanvasIsRendering(page);

    await page.evaluate(() => window.__TEST__!.tapButton('Start'));
    await page.waitForTimeout(2_500);

    // Auto-return from failed test — canvas should still render in build mode.
    await assertCanvasIsRendering(page, 500);
  });

  test('M4: pause menu opens and resumes', async ({ page }) => {
    await navigateToBuildMode(page);

    await page.evaluate(() => window.__TEST__!.tapButton('Select'));
    await assertCanvasChanged(page, 2_000);

    await page.evaluate(() => window.__TEST__!.tapButton('Start'));
    await page.waitForTimeout(1_000);
    await assertCanvasIsRendering(page);
  });

  test('M5: theater mode survives viewport resize', async ({ page }) => {
    await waitForRenderSettle(page, 3_000);

    await page.setViewportSize({ width: 1280, height: 720 });
    await page.locator('#btn-theater').click();
    await expect(page.locator('body')).toHaveClass(/theater-mode/);

    await page.setViewportSize({ width: 640, height: 480 });
    const metrics = await assertCanvasIsRendering(page, 800);
    expect(metrics.screenshotBytes).toBeGreaterThan(3_000);
  });
});

test.describe('Ship QA navigation smoke (M1 partial)', () => {
  test('M1 partial: reaches build mode from title', async ({ page }) => {
    await navigateToBuildMode(page);
    const metrics = await assertCanvasIsRendering(page);
    expect(metrics.screenshotBytes).toBeGreaterThan(3_000);
  });

  test('level select shows readable KNAPFORD track card', async ({ page }) => {
    await waitForRenderSettle(page, 3_000);
    await page.evaluate(() => window.__TEST__!.tapButton('Start'));
    await page.waitForTimeout(1_500);

    const shot = await page.locator('#game-canvas').screenshot();
    expect(shot.byteLength).toBeGreaterThan(3_000);
  });
});
