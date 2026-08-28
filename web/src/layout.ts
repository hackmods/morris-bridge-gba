export const GBA_WIDTH = 240;
export const GBA_HEIGHT = 160;

export function computeCanvasScale(
  containerWidth: number,
  containerHeight: number,
  maxScale = 6,
): number {
  if (containerWidth <= 0 || containerHeight <= 0) {
    return 3;
  }

  const scale = Math.min(
    containerWidth / GBA_WIDTH,
    containerHeight / GBA_HEIGHT,
    maxScale,
  );

  return Math.max(1, Math.floor(scale));
}

export function applyCanvasScale(canvas: HTMLCanvasElement, scale: number): void {
  canvas.style.width = `${GBA_WIDTH * scale}px`;
  canvas.style.height = `${GBA_HEIGHT * scale}px`;
}

export function bindResponsiveCanvas(
  canvas: HTMLCanvasElement,
  container: HTMLElement,
): () => void {
  const update = (): void => {
    const rect = container.getBoundingClientRect();
    const scale = computeCanvasScale(rect.width, rect.height);
    applyCanvasScale(canvas, scale);
  };

  update();

  const observer = new ResizeObserver(() => {
    update();
  });
  observer.observe(container);

  window.addEventListener('resize', update);

  return () => {
    observer.disconnect();
    window.removeEventListener('resize', update);
  };
}
