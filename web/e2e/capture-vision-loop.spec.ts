/**
 * Vision loop captures: EDIT chrome, SIM (reference via L+R+START), RESULT.
 * Avoid theater mode. Run:
 *   WALKTHROUGH_DIR=/opt/cursor/artifacts npx playwright test e2e/capture-vision-loop.spec.ts
 */
import { test } from '@playwright/test';
import {
  navigateToBuildMode,
  navigateToGordonBuildMode,
} from './helpers';

const OUT = process.env.WALKTHROUGH_DIR ?? '/opt/cursor/artifacts';

async function shot(page: import('@playwright/test').Page, name: string): Promise<void> {
  await page.locator('#game-canvas').screenshot({ path: `${OUT}/${name}.png` });
}

async function runReference(page: import('@playwright/test').Page): Promise<void> {
  await page.evaluate(() => window.__TEST__!.holdButtons(['L', 'R', 'Start'], 280));
  await page.waitForTimeout(500);
}

test('capture EDIT chrome on Knapford', async ({ page }) => {
  test.setTimeout(90_000);
  await navigateToBuildMode(page);
  await page.waitForTimeout(1_000);
  await shot(page, 'vision_edit_knapford');
  await shot(page, 'demo_edit_hud');
});

test('capture SIM fail RESULT via weak reference path', async ({ page }) => {
  test.setTimeout(180_000);
  await navigateToBuildMode(page);
  await page.waitForTimeout(600);
  // Empty Start → NO GO / stay edit; use L+R+START for a real deck then
  // we still want a fail: overlong isn't available via reference. Capture
  // mid-SIM stress on Gordon with a short wait after start, and fail via
  // Knapford by starting without a path then filling — prefer Gordon fail
  // from interrupting: just run Knapford reference and shoot early+late.
  await runReference(page);
  await shot(page, 'vision_sim_knap_0');
  for (let i = 1; i < 8; ++i) {
    await page.waitForTimeout(1_600);
    await shot(page, `vision_sim_knap_${i}`);
  }
  await page.waitForTimeout(4_000);
  await shot(page, 'vision_result_knap');
  await shot(page, 'vision_result_win');
  await shot(page, 'demo_result_win');
  await shot(page, 'demo_sim');
});

test('capture SIM on Gordon Hill with stress + RESULT', async ({ page }) => {
  test.setTimeout(240_000);
  await navigateToGordonBuildMode(page);
  await page.waitForTimeout(800);
  await shot(page, 'vision_edit_gordon');
  await runReference(page);
  await shot(page, 'vision_edit_gordon_built');
  for (let i = 0; i < 14; ++i) {
    await page.waitForTimeout(1_600);
    await shot(page, `vision_sim_gordon_${i}`);
  }
  await page.waitForTimeout(3_000);
  await shot(page, 'vision_result_win_gordon');
  await shot(page, 'demo_result_win');
  await shot(page, 'demo_sim');
});
