/**
 * One-shot capture: EDIT → SIM → RESULT on Knapford with overlong fail + reference win.
 * Run: npx playwright test e2e/capture-vision-loop.spec.ts --config=playwright.config.ts
 */
import { test } from '@playwright/test';
import {
  buildKnapfordReferenceBridge,
  buildOverlongKnapfordSpan,
  navigateToBuildMode,
  waitForRenderSettle,
} from './helpers';

const OUT = process.env.WALKTHROUGH_DIR ?? '/opt/cursor/artifacts';

async function tap(page: import('@playwright/test').Page, button: string, ms = 1_000): Promise<void> {
  await page.evaluate((b) => window.__TEST__!.tapButton(b), button);
  await page.waitForTimeout(ms);
}

test('capture EDIT then SIM fail RESULT', async ({ page }) => {
  test.setTimeout(180_000);
  await navigateToBuildMode(page);
  await page.locator('#btn-theater').click().catch(() => undefined);
  await page.waitForTimeout(400);
  await page.locator('#game-canvas').screenshot({ path: `${OUT}/vision_edit_knapford.png` });

  await buildOverlongKnapfordSpan(page);
  await page.waitForTimeout(800);
  await page.locator('#game-canvas').screenshot({ path: `${OUT}/vision_edit_weak_span.png` });

  await tap(page, 'Start', 500);
  await page.waitForTimeout(2_500);
  await page.locator('#game-canvas').screenshot({ path: `${OUT}/vision_sim_running_or_fail.png` });
  await page.waitForTimeout(4_000);
  await page.locator('#game-canvas').screenshot({ path: `${OUT}/vision_sim_mid_fail.png` });
  await page.waitForTimeout(8_000);
  await page.locator('#game-canvas').screenshot({ path: `${OUT}/vision_result_fail.png` });
});

test('capture EDIT then SIM win RESULT', async ({ page }) => {
  test.setTimeout(180_000);
  await navigateToBuildMode(page);
  await page.locator('#btn-theater').click().catch(() => undefined);
  await buildKnapfordReferenceBridge(page);
  await page.waitForTimeout(800);
  await page.locator('#game-canvas').screenshot({ path: `${OUT}/vision_edit_reference.png` });

  await tap(page, 'Start', 500);
  await page.waitForTimeout(3_000);
  await page.locator('#game-canvas').screenshot({ path: `${OUT}/vision_sim_crossing.png` });
  await page.waitForTimeout(6_000);
  await page.locator('#game-canvas').screenshot({ path: `${OUT}/vision_sim_crossing_late.png` });
  await page.waitForTimeout(12_000);
  await page.locator('#game-canvas').screenshot({ path: `${OUT}/vision_result_win.png` });
});
