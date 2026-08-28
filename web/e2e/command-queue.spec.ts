import { test, expect } from '@playwright/test';
import { assertEmulatorStillRunning, waitForPlaytestReady, waitForRenderSettle } from './helpers';

test.describe('Command queue', () => {
  test('enqueues Start and shows it in the live feed', async ({ page }) => {
    await waitForPlaytestReady(page);

    await page.locator('#command-input').fill('start');
    await page.locator('#btn-enqueue').click();

    await expect(page.locator('#input-feed li').first()).toContainText('Start');
  });

  test('processes multi-token input without freezing the emulator', async ({ page }) => {
    await waitForRenderSettle(page);

    await page.locator('#command-input').fill('right a start');
    await page.locator('#btn-enqueue').click();

    await expect(page.locator('#input-feed')).toContainText('Start');
    await assertEmulatorStillRunning(page);
  });

  test('pause and clear queue controls work', async ({ page }) => {
    await waitForPlaytestReady(page);

    await page.locator('#command-input').fill('up down left right');
    await page.locator('#btn-enqueue').click();
    await page.locator('#btn-pause-queue').click();

    await expect(page.locator('#btn-pause-queue')).toHaveText('Resume queue');
    await expect(page.locator('#input-feed')).toContainText('Queue paused');

    await page.locator('#btn-clear-queue').click();
    await expect(page.locator('#input-feed')).toContainText('Queue cleared');
  });
});
