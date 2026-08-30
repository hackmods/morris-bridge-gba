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
      timeout: 30_000,
    });
    await expect(page.locator('#result-title')).toContainText('The train is through');
    const stars = await page.evaluate(() => window.__GAME__?.getStars());
    expect(stars).toBeGreaterThanOrEqual(1);
  });

  test('drag-weld from the left bank creates a beam', async ({ page }) => {
    await enterBuild(page);
    const before = await page.evaluate(() => window.__GAME__?.snapshot().beams);
    const canvas = page.locator('#world');
    const box = await canvas.boundingBox();
    expect(box).toBeTruthy();
    if (!box) {
      return;
    }
    const start = { x: box.x + box.width * 0.27, y: box.y + box.height * 0.58 };
    const mid = { x: box.x + box.width * 0.42, y: box.y + box.height * 0.58 };
    await page.mouse.move(start.x, start.y);
    await page.mouse.down();
    await page.mouse.move(mid.x, mid.y, { steps: 12 });
    await page.mouse.up();
    await page.waitForTimeout(200);
    const after = await page.evaluate(() => window.__GAME__?.snapshot().beams);
    expect(Number(after)).toBeGreaterThan(Number(before));
  });
});
