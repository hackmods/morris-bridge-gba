import { test } from '@playwright/test';
import { waitForRenderSettle } from './helpers';

const OUT_DIR = process.env.WALKTHROUGH_DIR ?? 'test-results/walkthrough';

async function tap(page: import('@playwright/test').Page, button: string, ms = 1_200): Promise<void> {
  await page.evaluate((b) => window.__TEST__!.tapButton(b), button);
  await page.waitForTimeout(ms);
}

/** Title → level select → engine select → (intro) → Knapford build on fresh SRAM. */
test('capture Knapford walkthrough screenshots', async ({ page }) => {
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

  await tap(page, 'Start');
  await page.waitForTimeout(1_500);
  await page.locator('#game-canvas').screenshot({ path: `${OUT_DIR}/04_knapford_build.png` });
});

/** Session goal: Gordon's Hill build with brace tool + hill BG theme (save unlock injected). */
test('capture Gordon Hill walkthrough screenshots', async ({ page }) => {
  await page.goto('/?walkthrough=gordon');
  await waitForRenderSettle(page, 3_000);
  await page.locator('#btn-theater').click();
  await page.waitForTimeout(500);

  await tap(page, 'Start');
  await tap(page, 'Down');
  await page.waitForTimeout(800);
  await page.locator('#game-canvas').screenshot({ path: `${OUT_DIR}/05_gordons_hill_select.png` });

  await tap(page, 'Start');
  await tap(page, 'Start');
  await page.waitForTimeout(1_500);
  await page.locator('#game-canvas').screenshot({ path: `${OUT_DIR}/06_gordons_hill_build_track.png` });

  await tap(page, 'R');
  await page.waitForTimeout(800);
  await page.locator('#game-canvas').screenshot({ path: `${OUT_DIR}/07_gordons_hill_build_rod.png` });
});

/** Track 3, Culdee Fell (all_unlocked walkthrough save; file names kept from when it was Vicarstown). */
test('capture Culdee Fell walkthrough screenshots', async ({ page }) => {
  await page.goto('/?walkthrough=vicarstown');
  await waitForRenderSettle(page, 3_000);
  await page.locator('#btn-theater').click();
  await page.waitForTimeout(500);

  await tap(page, 'Start');
  await tap(page, 'Down');
  await tap(page, 'Down');
  await page.waitForTimeout(800);
  await page.locator('#game-canvas').screenshot({ path: `${OUT_DIR}/08_vicarstown_select.png` });

  await tap(page, 'Start');
  await tap(page, 'Start');
  await page.waitForTimeout(1_500);
  await page.locator('#game-canvas').screenshot({ path: `${OUT_DIR}/09_vicarstown_build.png` });
});
