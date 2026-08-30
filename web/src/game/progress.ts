import { ENGINES, type EngineId } from './engines';
import { LEVELS } from './levels';

const KEY = 'thomas-bridge-progress-v1';

export interface Progress {
  version: number;
  maxUnlockedLevel: number;
  stars: number[];
  bestCost: number[];
  seenTutorial: boolean;
}

export function emptyProgress(): Progress {
  return {
    version: 1,
    maxUnlockedLevel: 0,
    stars: LEVELS.map(() => 0),
    bestCost: LEVELS.map(() => 0),
    seenTutorial: false,
  };
}

export function loadProgress(): Progress {
  try {
    const raw = localStorage.getItem(KEY);
    if (!raw) {
      return emptyProgress();
    }
    const parsed = JSON.parse(raw) as Progress;
    const base = emptyProgress();
    return {
      ...base,
      ...parsed,
      stars: LEVELS.map((_, i) => parsed.stars?.[i] ?? 0),
      bestCost: LEVELS.map((_, i) => parsed.bestCost?.[i] ?? 0),
      maxUnlockedLevel: Math.min(
        LEVELS.length - 1,
        Math.max(0, parsed.maxUnlockedLevel ?? 0),
      ),
    };
  } catch {
    return emptyProgress();
  }
}

export function saveProgress(progress: Progress): void {
  localStorage.setItem(KEY, JSON.stringify(progress));
}

export function recordClear(progress: Progress, levelIndex: number, stars: number, cost: number): Progress {
  const next = {
    ...progress,
    stars: [...progress.stars],
    bestCost: [...progress.bestCost],
  };
  next.stars[levelIndex] = Math.max(next.stars[levelIndex] ?? 0, stars);
  const previous = next.bestCost[levelIndex] ?? 0;
  next.bestCost[levelIndex] = previous === 0 ? cost : Math.min(previous, cost);
  next.maxUnlockedLevel = Math.max(next.maxUnlockedLevel, Math.min(LEVELS.length - 1, levelIndex + 1));
  return next;
}

export function isEngineUnlocked(progress: Progress, engineId: EngineId): boolean {
  const engine = ENGINES.find((item) => item.id === engineId);
  if (!engine) {
    return false;
  }
  if (engine.unlockAfterLevel < 0) {
    return true;
  }
  return (progress.stars[engine.unlockAfterLevel] ?? 0) > 0;
}

export function isLevelUnlocked(progress: Progress, index: number): boolean {
  return index <= progress.maxUnlockedLevel;
}
