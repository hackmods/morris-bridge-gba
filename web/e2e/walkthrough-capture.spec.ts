import { test } from '@playwright/test';
import { waitForRenderSettle } from './helpers';

const OUT_DIR = process.env.WALKTHROUGH_DIR ?? 'test-results/walkthrough';

async function tap(page: import('@playwright/test').Page, button: string, ms = 1_200): Promise<void> {
  await page.evaluate((b) => window.__TEST__!.tapButton(b), button);
  await page.waitForTimeout(ms);
}

/** Title → level select → engine select → (intro) → Knapford build on fresh SRAM. */
test('capture Phase 7 ship walkthrough screenshots', async ({ page }) => {
  await waitForRenderSettle(page, 3_000);
  await page.locator('#btn-theater').click();
  await page.waitForTimeout(500);

  await page.locator('#game-canvas').screenshot({ path: `${OUT_DIR}/01_title_screen.png` });

  await tap(page, 'Start');
  await page.locator('#game-canvas').screenshot({ path: `${OUT_DIR}/02_level_select.png` });

  await tap(page, 'Start');
  await page.locator('#game-canvas').screenshot({ path: `${OUT_DIR}/03_engine_select.png` });

  await tap(page, 'Start');
  await page.waitForTimeout(800);

  // First-visit level intro (Knapford dialog).
  await tap(page, 'Start');
  await page.waitForTimeout(1_500);
  await page.locator('#game-canvas').screenshot({ path: `${OUT_DIR}/04_knapford_build.png` });

  await tap(page, 'Start');
  await page.waitForTimeout(2_000);
  await page.locator('#game-canvas').screenshot({ path: `${OUT_DIR}/05_knapford_test_no_go.png` });
});
