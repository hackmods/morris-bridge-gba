/**
 * One-shot capture: EDIT → SIM → RESULT on Knapford with overlong fail + reference win.
 * Avoid theater mode (can hide chrome). Run:
 *   npx playwright test e2e/capture-vision-loop.spec.ts --config=playwright.config.ts
 */
import { test } from '@playwright/test';
import {
  buildKnapfordReferenceBridge,
  buildOverlongKnapfordSpan,
  navigateToBuildMode,
} from './helpers';

const OUT = process.env.WALKTHROUGH_DIR ?? '/opt/cursor/artifacts';

async function tap(page: import('@playwright/test').Page, button: string, ms = 1_000): Promise<void> {
  await page.evaluate((b) => window.__TEST__!.tapButton(b), button);
  await page.waitForTimeout(ms);
}

async function shot(page: import('@playwright/test').Page, name: string): Promise<void> {
  await page.locator('#game-canvas').screenshot({ path: `${OUT}/${name}.png` });
}

test('capture EDIT then SIM fail RESULT', async ({ page }) => {
  test.setTimeout(180_000);
  await navigateToBuildMode(page);
  await page.waitForTimeout(800);
  await shot(page, 'vision_edit_knapford');

  await buildOverlongKnapfordSpan(page);
  await page.waitForTimeout(800);
  await shot(page, 'vision_edit_weak_span');

  await tap(page, 'Start', 400);
  await page.waitForTimeout(2_000);
  await shot(page, 'vision_sim_running_or_fail');
  await page.waitForTimeout(4_000);
  await shot(page, 'vision_sim_mid_fail');
  await page.waitForTimeout(10_000);
  await shot(page, 'vision_result_fail');
});

test('capture EDIT then SIM win RESULT', async ({ page }) => {
  test.setTimeout(180_000);
  await navigateToBuildMode(page);
  await buildKnapfordReferenceBridge(page);
  await page.waitForTimeout(800);
  await shot(page, 'vision_edit_reference');

  await tap(page, 'Start', 400);
  await page.waitForTimeout(2_500);
  await shot(page, 'vision_sim_crossing');
  await page.waitForTimeout(5_000);
  await shot(page, 'vision_sim_crossing_late');
  await page.waitForTimeout(14_000);
  await shot(page, 'vision_result_win');
});
