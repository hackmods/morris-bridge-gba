import { test, expect } from '@playwright/test';
import { waitForRenderSettle } from './helpers';

test('walkthrough=gordon seeds Gordon Hill on level select', async ({ page }) => {
  await page.goto('/legacy.html?walkthrough=gordon');
  await waitForRenderSettle(page, 3_000);

  const level = await page.evaluate(() => window.__TEST__!.readWalkthroughSaveLevel());
  expect(level).toBe(1);

  await page.evaluate(() => window.__TEST__!.tapButton('Start'));
  await page.waitForTimeout(2_000);

  const before = await page.locator('#game-canvas').screenshot();
  await page.evaluate(() => window.__TEST__!.tapButton('Down'));
  await page.waitForTimeout(1_200);
  const after = await page.locator('#game-canvas').screenshot();

  expect(before.equals(after)).toBe(false);
});
