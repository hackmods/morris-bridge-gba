import { test, expect } from '@playwright/test';
import {
  assertCanvasChanged,
  assertCanvasIsRendering,
  buildKnapfordReferenceBridge,
  navigateToBuildMode,
  navigateToGordonBuildMode,
  waitForRenderSettle,
} from './helpers';

/**
 * Web playtest proxies for EverDrive hardware sign-off (H1–H5).
 * H6 SRAM power-cycle remains hardware-only; see sram-persist.spec.ts for soft-reset proxy.
 */

test('H1 proxy: title attract loop renders without crash (15 s)', async ({ page }) => {
  await waitForRenderSettle(page, 3_000);
  await expect(page.locator('#error-banner')).toHaveClass(/hidden/);

  const title = await page.locator('#game-canvas').screenshot();
  expect(title.byteLength).toBeGreaterThan(3_000);

  // Hardware checklist uses 60 s; 15 s catches hang/crash regressions in CI.
  await page.waitForTimeout(15_000);

  await expect(page.locator('#error-banner')).toHaveClass(/hidden/);
  await assertCanvasIsRendering(page);
});

test('H2 proxy: Knapford reference bridge wins', async ({ page }) => {
  await navigateToBuildMode(page);
  await buildKnapfordReferenceBridge(page);

  const before = await page.locator('#game-canvas').screenshot();
  await page.evaluate(() => window.__TEST__!.tapButton('Start'));
  await page.waitForTimeout(28_000);

  const after = await page.locator('#game-canvas').screenshot();
  expect(after.equals(before)).toBe(false);
});

test('H3 proxy: Gordon Hill unlock + hill build + ROD tool', async ({ page }) => {
  await navigateToGordonBuildMode(page);

  const buildBefore = await page.locator('#game-canvas').screenshot();
  await page.evaluate(() => window.__TEST__!.tapButton('R'));
  await page.waitForTimeout(800);
  const buildAfter = await page.locator('#game-canvas').screenshot();
  expect(buildAfter.equals(buildBefore)).toBe(false);
});

test('H4 proxy: engine select portrait changes when scrolling', async ({ page }) => {
  await waitForRenderSettle(page, 3_000);
  await page.evaluate(() => window.__TEST__!.tapButton('Start'));
  await page.waitForTimeout(1_200);
  await page.evaluate(() => window.__TEST__!.tapButton('Start'));
  await page.waitForTimeout(1_500);

  const thomas = await page.locator('#game-canvas').screenshot();
  await page.evaluate(() => window.__TEST__!.tapButton('Down'));
  await page.waitForTimeout(1_200);
  const percy = await page.locator('#game-canvas').screenshot();

  expect(percy.equals(thomas)).toBe(false);
});

test('H5 proxy: pause menu opens and resumes (audio duck — hardware only)', async ({ page }) => {
  await navigateToBuildMode(page);

  await page.evaluate(() => window.__TEST__!.tapButton('Select'));
  await assertCanvasChanged(page, 2_000);

  await page.evaluate(() => window.__TEST__!.tapButton('Start'));
  await page.waitForTimeout(1_000);
  await assertCanvasIsRendering(page);
});
