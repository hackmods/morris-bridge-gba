import { expect, type Page, test } from '@playwright/test';

async function openWorkshop(page: Page): Promise<void> {
  await page.goto('/');
  await expect(page.locator('#screen-title h1')).toContainText("Thomas's Bridge");
  await page.locator('#btn-play').click();
  await expect(page.locator('#screen-levels')).toBeVisible();
}

async function enterBuild(page: Page): Promise<void> {
  await openWorkshop(page);
  await page.locator('#level-grid .card').first().click();
  await expect(page.locator('#screen-engines')).toBeVisible();
  await page.locator('#btn-engines-go').click();
  await expect(page.locator('#hud')).toBeVisible();
  await page.waitForFunction(() => window.__GAME__?.getScreen() === 'build');
}

test.describe('Thomas\'s Bridge workshop', () => {
  test('title opens a modern workshop, not a GBA emulator', async ({ page }) => {
    await page.goto('/');
    await expect(page.locator('#screen-title h1')).toHaveText("Thomas's Bridge");
    await expect(page.locator('#btn-play')).toBeVisible();
    await expect(page.locator('#world')).toBeVisible();
    await expect(page.locator('#game-canvas')).toHaveCount(0);
  });

  test('campaign lists eight crossings', async ({ page }) => {
    await openWorkshop(page);
    await expect(page.locator('#level-grid .card')).toHaveCount(8);
    await expect(page.locator('#level-grid .card').first()).toContainText('Knapford Creek');
    await expect(page.locator('#level-grid .card').last()).toContainText('The Express');
  });

  test('cannot test until a road is welded across', async ({ page }) => {
    await enterBuild(page);
    const status = await page.evaluate(() => window.__GAME__?.startTest());
    expect(status).toBe('no_path');
    await expect(page.locator('#toast')).toContainText('Connect a continuous road');
  });

  test('Knapford reference bridge wins a crossing', async ({ page }) => {
    await enterBuild(page);
    const built = await page.evaluate(() => window.__GAME__?.buildReference());
    expect(built).toBe(true);
    await page.evaluate(() => window.__GAME__?.startTest());
    await page.waitForFunction(() => window.__GAME__?.getScreen() === 'result', null, {
      timeout: 20_000,
    });
    await expect(page.locator('#result-title')).toContainText('The train is through');
    const stars = await page.evaluate(() => window.__GAME__?.getStars());
    expect(stars).toBeGreaterThanOrEqual(1);
  });

  test('every official crossing has a reference that wins', async ({ page }) => {
    test.setTimeout(120_000);
    await page.goto('/');
    await page.evaluate(() => {
      localStorage.setItem(
        'thomas-bridge-progress-v1',
        JSON.stringify({
          version: 1,
          maxUnlockedLevel: 7,
          stars: [3, 3, 3, 3, 3, 3, 3, 3],
          bestCost: [1, 1, 1, 1, 1, 1, 1, 1],
          seenTutorial: true,
        }),
      );
    });
    await page.reload();
    await expect(page.locator('#btn-play')).toBeVisible();
    await page.locator('#btn-play').click();

    const ids = await page.evaluate(async () => {
      const game = window.__GAME__;
      if (!game) {
        return [];
      }
      const results: string[] = [];
      for (let index = 0; index < 8; index += 1) {
        game.selectLevel(index);
        game.goBuild();
        if (!game.buildReference()) {
          results.push(`build-fail:${index}`);
          continue;
        }
        game.startTest();
        const started = Date.now();
        while (game.getScreen() !== 'result' && Date.now() - started < 25_000) {
          await new Promise((resolve) => setTimeout(resolve, 50));
        }
        results.push(`${game.getLevelId()}:${game.getScreen()}:${game.getStars()}`);
      }
      return results;
    });

    expect(ids).toHaveLength(8);
    for (const row of ids) {
      expect(row, row).toMatch(/:result:[1-3]$/);
    }
  });

  test('drag-weld from the left bank creates a beam', async ({ page }) => {
    await enterBuild(page);
    const before = await page.evaluate(() => Number(window.__GAME__?.snapshot().beams));
    const canvas = page.locator('#world');
    const box = await canvas.boundingBox();
    expect(box).toBeTruthy();
    if (!box) {
      return;
    }
    const points = await page.evaluate(() => {
      const game = window.__GAME__;
      if (!game) {
        return null;
      }
      const start = game.worldToScreen(262, 382);
      const mid = game.worldToScreen(440, 382);
      return { start, mid };
    });
    expect(points).toBeTruthy();
    if (!points) {
      return;
    }
    await page.mouse.move(box.x + points.start.x, box.y + points.start.y);
    await page.mouse.down();
    await page.mouse.move(box.x + points.mid.x, box.y + points.mid.y, { steps: 12 });
    await page.mouse.up();
    await page.waitForTimeout(250);
    const after = await page.evaluate(() => Number(window.__GAME__?.snapshot().beams));
    expect(after).toBeGreaterThan(before);
  });

  test('one long drag chain-welds across Knapford Creek', async ({ page }) => {
    await enterBuild(page);
    const canvas = page.locator('#world');
    const box = await canvas.boundingBox();
    expect(box).toBeTruthy();
    if (!box) {
      return;
    }
    const points = await page.evaluate(() => {
      const game = window.__GAME__;
      if (!game) {
        return null;
      }
      return {
        start: game.worldToScreen(262, 382),
        end: game.worldToScreen(798, 382),
      };
    });
    expect(points).toBeTruthy();
    if (!points) {
      return;
    }
    await page.mouse.move(box.x + points.start.x, box.y + points.start.y);
    await page.mouse.down();
    await page.mouse.move(box.x + points.end.x, box.y + points.end.y, { steps: 24 });
    await page.mouse.up();
    await page.waitForTimeout(300);
    const snap = await page.evaluate(() => window.__GAME__?.snapshot());
    expect(Number(snap?.beams)).toBeGreaterThanOrEqual(2);
    expect(snap?.path).toBe(true);
  });
});
