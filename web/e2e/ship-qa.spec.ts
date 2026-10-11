import { test, expect } from '@playwright/test';
import {
  assertCanvasChanged,
  assertCanvasIsRendering,
  buildGordonHillReferenceBridge,
  buildKnapfordReferenceBridge,
  buildUnsupportedKnapfordDeck,
  buildCuldeeFellReferenceBridge,
  navigateToBuildMode,
  navigateToGordonBuildMode,
  navigateToCuldeeFellBuildMode,
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

  test('M4: pause menu returns to level select', async ({ page }) => {
    await navigateToBuildMode(page);

    await page.evaluate(() => window.__TEST__!.tapButton('Select'));
    await page.waitForTimeout(800);
    await page.evaluate(() => window.__TEST__!.tapButton('Down'));
    await page.evaluate(() => window.__TEST__!.tapButton('Down'));
    await page.waitForTimeout(300);
    await page.evaluate(() => window.__TEST__!.tapButton('Start'));
    await page.waitForTimeout(1_500);

    await assertCanvasIsRendering(page, 500);
  });

  test('M4: pause menu returns to title', async ({ page }) => {
    await navigateToBuildMode(page);

    await page.evaluate(() => window.__TEST__!.tapButton('Select'));
    await page.waitForTimeout(800);
    for (let i = 0; i < 3; i += 1) {
      await page.evaluate(() => window.__TEST__!.tapButton('Down'));
      await page.waitForTimeout(200);
    }
    await page.evaluate(() => window.__TEST__!.tapButton('Start'));
    await page.waitForTimeout(2_000);

    await assertCanvasIsRendering(page, 500);
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

test.describe('Ship QA navigation smoke (M1–M2)', () => {
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

  test('M1: Knapford reference bridge wins', async ({ page }) => {
    test.setTimeout(180_000);

    await navigateToBuildMode(page);
    await buildKnapfordReferenceBridge(page);

    const beforeTest = await page.locator('#game-canvas').screenshot();
    await page.evaluate(() => window.__TEST__!.tapButton('Start'));
    await page.waitForTimeout(18_000);

    const afterTest = await page.locator('#game-canvas').screenshot();
    expect(beforeTest.equals(afterTest)).toBe(false);
    // A real win, not just a changed picture: the save now has Knapford cleared.
    const save = await page.evaluate(() => window.__TEST__!.readSaveProgress());
    expect((save?.clearedLevels ?? 0) & 1).toBe(1);
    await assertCanvasIsRendering(page, 500);
  });

  test('M2: unsupported Knapford deck fails test run', async ({ page }) => {
    test.setTimeout(120_000);

    await navigateToBuildMode(page);
    await buildUnsupportedKnapfordDeck(page);

    const beforeTest = await page.locator('#game-canvas').screenshot();
    await page.evaluate(() => window.__TEST__!.tapButton('Start'));
    await page.waitForTimeout(12_000);

    const afterTest = await page.locator('#game-canvas').screenshot();
    expect(beforeTest.equals(afterTest)).toBe(false);
    // And a real failure: nothing was cleared.
    const save = await page.evaluate(() => window.__TEST__!.readSaveProgress());
    expect((save?.clearedLevels ?? 0) & 1).toBe(0);
    await assertCanvasIsRendering(page, 500);
  });

  // Not a win check: timed D-pad input drifts a few pixels per move, which a
  // 13-step build does not survive reliably. Knapford (M1/M2) asserts the real
  // outcome from the save; these two only prove the build and run do not hang.
  test("Gordon's Hill: build and run with walkthrough save", async ({ page }) => {
    test.setTimeout(240_000);

    await navigateToGordonBuildMode(page);
    await buildGordonHillReferenceBridge(page);

    const beforeTest = await page.locator('#game-canvas').screenshot();
    await page.evaluate(() => window.__TEST__!.tapButton('Start'));
    await page.waitForTimeout(22_000);

    const afterTest = await page.locator('#game-canvas').screenshot();
    expect(beforeTest.equals(afterTest)).toBe(false);
    await assertCanvasIsRendering(page, 500);
  });

  test("Culdee Fell: build and run with walkthrough save", async ({ page }) => {
    test.setTimeout(270_000);

    await navigateToCuldeeFellBuildMode(page);
    await buildCuldeeFellReferenceBridge(page);

    const beforeTest = await page.locator('#game-canvas').screenshot();
    await page.evaluate(() => window.__TEST__!.tapButton('Start'));
    await page.waitForTimeout(25_000);

    const afterTest = await page.locator('#game-canvas').screenshot();
    expect(beforeTest.equals(afterTest)).toBe(false);
    await assertCanvasIsRendering(page, 500);
  });
});
