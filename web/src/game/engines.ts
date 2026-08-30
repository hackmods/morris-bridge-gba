export type EngineId = 'thomas' | 'percy' | 'edward' | 'james' | 'gordon';

export interface EngineProfile {
  id: EngineId;
  name: string;
  title: string;
  color: string;
  cab: string;
  weight: number;
  speed: number;
  coaches: number;
  unlockAfterLevel: number;
  blurb: string;
}

export const ENGINES: EngineProfile[] = [
  {
    id: 'thomas',
    name: 'Thomas',
    title: 'No. 1 tank engine',
    color: '#3d8fd1',
    cab: '#c45c4a',
    weight: 4,
    speed: 1,
    coaches: 0,
    unlockAfterLevel: -1,
    blurb: 'A balanced first crossing. Forgiving on short decks.',
  },
  {
    id: 'percy',
    name: 'Percy',
    title: 'No. 6 saddletank',
    color: '#4f9a4a',
    cab: '#c45c4a',
    weight: 3,
    speed: 1.08,
    coaches: 0,
    unlockAfterLevel: 0,
    blurb: 'Lightest engine. Use him when a span is looking thin.',
  },
  {
    id: 'edward',
    name: 'Edward',
    title: 'No. 2 tender engine',
    color: '#3c6ea8',
    cab: '#c45c4a',
    weight: 5,
    speed: 0.92,
    coaches: 1,
    unlockAfterLevel: 2,
    blurb: 'Steady and heavier. Bracing starts to matter.',
  },
  {
    id: 'james',
    name: 'James',
    title: 'No. 5 red engine',
    color: '#c45c4a',
    cab: '#2a3038',
    weight: 6,
    speed: 1,
    coaches: 1,
    unlockAfterLevel: 4,
    blurb: 'A serious load. Steel or tight triangles, or he will snap the deck.',
  },
  {
    id: 'gordon',
    name: 'Gordon',
    title: 'No. 4 express',
    color: '#3d6fb3',
    cab: '#c45c4a',
    weight: 10,
    speed: 1.18,
    coaches: 2,
    unlockAfterLevel: 7,
    blurb: 'Prestige run. If the bridge holds Gordon, it holds anything.',
  },
];

export function engineById(id: EngineId): EngineProfile {
  const found = ENGINES.find((engine) => engine.id === id);
  if (!found) {
    throw new Error(`Unknown engine ${id}`);
  }
  return found;
}
