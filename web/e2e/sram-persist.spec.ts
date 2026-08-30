import { test, expect } from '@playwright/test';
import { navigateToBuildMode } from './helpers';

/** H6 proxy: emulated SRAM intro_seen survives soft reset (mGBA-WASM). */
test('intro_seen persists across soft reset', async ({ page }) => {
  await navigateToBuildMode(page);

  const beforeReset = await page.evaluate(() => window.__TEST__!.readSaveProgress());
  expect(beforeReset).not.toBeNull();
  expect(beforeReset!.version).toBe(4);
  expect(beforeReset!.introSeenMask & 0b001).toBe(0b001);

  await page.locator('#btn-reset').click();
  await page.waitForTimeout(2_500);

  const afterReset = await page.evaluate(() => window.__TEST__!.readSaveProgress());
  expect(afterReset).not.toBeNull();
  expect(afterReset!.introSeenMask).toBe(beforeReset!.introSeenMask);
});

test('walkthrough save intro_seen mask survives soft reset', async ({ page }) => {
  await page.goto('/?walkthrough=gordon');
  await page.waitForTimeout(3_000);
  await expect(page.locator('#status-bar')).toHaveClass(/ready/, { timeout: 60_000 });

  const beforeReset = await page.evaluate(() => window.__TEST__!.readSaveProgress());
  expect(beforeReset?.introSeenMask).toBe(0b011);

  await page.locator('#btn-reset').click();
  await page.waitForTimeout(2_500);

  const afterReset = await page.evaluate(() => window.__TEST__!.readSaveProgress());
  expect(afterReset?.introSeenMask).toBe(0b011);
  expect(afterReset?.maxUnlockedLevel).toBe(1);
});

/** H6 proxy: stars + clear flags from all_unlocked walkthrough save survive soft reset. */
test('walkthrough save stars and clear flags survive soft reset', async ({ page }) => {
  await page.goto('/?walkthrough=vicarstown');
  await page.waitForTimeout(3_000);
  await expect(page.locator('#status-bar')).toHaveClass(/ready/, { timeout: 60_000 });

  const beforeReset = await page.evaluate(() => window.__TEST__!.readSaveProgress());
  expect(beforeReset?.clearedLevels).toBe(0xFF);
  expect(beforeReset?.packedStars).toBe(0xFFFF);
  expect(beforeReset?.maxUnlockedLevel).toBe(7);
  expect(beforeReset?.introSeenMask).toBe(0xFF);

  await page.locator('#btn-reset').click();
  await page.waitForTimeout(2_500);

  const afterReset = await page.evaluate(() => window.__TEST__!.readSaveProgress());
  expect(afterReset?.clearedLevels).toBe(beforeReset?.clearedLevels);
  expect(afterReset?.packedStars).toBe(beforeReset?.packedStars);
  expect(afterReset?.introSeenMask).toBe(beforeReset?.introSeenMask);
});
