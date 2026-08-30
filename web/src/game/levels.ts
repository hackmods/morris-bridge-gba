import type { MaterialId } from './materials';
import type { TerrainPad } from './physics';

export interface LevelTheme {
  skyTop: string;
  skyHorizon: string;
  sun: string;
  farHill: string;
  midHill: string;
  grass: string;
  grassDark: string;
  dirt: string;
  dirtDark: string;
  water: string;
  waterDeep: string;
}

export interface LevelDef {
  id: string;
  name: string;
  region: string;
  briefing: string;
  tools: MaterialId[];
  parCost: number;
  budget: number;
  width: number;
  height: number;
  waterY: number;
  wind: number;
  terrain: TerrainPad[];
  anchors: { x: number; y: number }[];
  start: { x: number; y: number };
  end: { x: number; y: number };
  theme: LevelTheme;
}

const MEADOW: LevelTheme = {
  skyTop: '#7eb7de',
  skyHorizon: '#f3d7a4',
  sun: '#ffe7a8',
  farHill: '#8fb56e',
  midHill: '#6f9a52',
  grass: '#5d9a4a',
  grassDark: '#3f7a36',
  dirt: '#c4a06a',
  dirtDark: '#8a6a3c',
  water: '#3e8fb0',
  waterDeep: '#1f5f86',
};

const HILL: LevelTheme = {
  skyTop: '#6a90b8',
  skyHorizon: '#d9c4a0',
  sun: '#f0d090',
  farHill: '#6d7f55',
  midHill: '#5a6a44',
  grass: '#4e7d3c',
  grassDark: '#35582b',
  dirt: '#b08a58',
  dirtDark: '#6e5230',
  water: '#3a7fa4',
  waterDeep: '#1a4e70',
};

const DOCK: LevelTheme = {
  skyTop: '#6aa0c4',
  skyHorizon: '#f0c9a0',
  sun: '#ffd39a',
  farHill: '#7a8c6a',
  midHill: '#5d6e52',
  grass: '#4f8a4a',
  grassDark: '#2f6230',
  dirt: '#b49264',
  dirtDark: '#6d4e30',
  water: '#2f7aa8',
  waterDeep: '#163e6c',
};

const VIADUCT: LevelTheme = {
  skyTop: '#5b7fa8',
  skyHorizon: '#c9b8c4',
  sun: '#f0c8b0',
  farHill: '#6a6a62',
  midHill: '#54564e',
  grass: '#4a6e44',
  grassDark: '#2f4a2e',
  dirt: '#8e7a68',
  dirtDark: '#5a4e42',
  water: '#2c6288',
  waterDeep: '#14344e',
};

const STORM: LevelTheme = {
  skyTop: '#4d6580',
  skyHorizon: '#b7b3a8',
  sun: '#d8c8a0',
  farHill: '#5a6758',
  midHill: '#3f4c40',
  grass: '#3d6a40',
  grassDark: '#2a4a2e',
  dirt: '#8a7054',
  dirtDark: '#4e3e2c',
  water: '#2a5e80',
  waterDeep: '#10283c',
};

function banks(
  left: number,
  right: number,
  deck: number,
  worldW: number,
  worldH: number,
): TerrainPad[] {
  return [
    { x0: -80, x1: left, y: deck },
    { x0: right, x1: worldW + 80, y: deck },
    { x0: -80, x1: left, y: worldH },
    { x0: right, x1: worldW + 80, y: worldH },
  ];
}

export const LEVELS: LevelDef[] = [
  {
    id: 'knapford',
    name: 'Knapford Creek',
    region: 'The Island of Sodor',
    briefing: 'Drag a road from the left bank to the right. The train only travels on road.',
    tools: ['road', 'wood'],
    parCost: 2100,
    budget: 2800,
    width: 1180,
    height: 640,
    waterY: 470,
    wind: 0,
    terrain: banks(300, 760, 392, 1180, 640),
    start: { x: 262, y: 382 },
    end: { x: 798, y: 382 },
    anchors: [
      { x: 262, y: 382 },
      { x: 798, y: 382 },
      { x: 210, y: 382 },
      { x: 850, y: 382 },
      { x: 236, y: 455 },
      { x: 824, y: 455 },
    ],
    theme: MEADOW,
  },
  {
    id: 'tidmouth',
    name: 'Tidmouth Gap',
    region: 'West coast',
    briefing: 'A wider creek. Split the deck, then add timber triangles so it does not sag.',
    tools: ['road', 'wood'],
    parCost: 1280,
    budget: 3600,
    width: 1280,
    height: 640,
    waterY: 478,
    wind: 0,
    terrain: banks(280, 920, 388, 1280, 640),
    start: { x: 248, y: 378 },
    end: { x: 952, y: 378 },
    anchors: [
      { x: 248, y: 378 },
      { x: 952, y: 378 },
      { x: 196, y: 378 },
      { x: 1004, y: 378 },
      { x: 220, y: 458 },
      { x: 980, y: 458 },
    ],
    theme: MEADOW,
  },
  {
    id: 'hill',
    name: "Gordon's Hill",
    region: 'East line',
    briefing: 'The deck sits higher than the banks. Timber braces from the lower anchors will save it.',
    tools: ['road', 'wood'],
    parCost: 1540,
    budget: 4000,
    width: 1320,
    height: 680,
    waterY: 500,
    wind: 0,
    terrain: [
      { x0: -80, x1: 270, y: 430 },
      { x0: 980, x1: 1400, y: 430 },
    ],
    start: { x: 250, y: 348 },
    end: { x: 1000, y: 348 },
    anchors: [
      { x: 250, y: 348 },
      { x: 1000, y: 348 },
      { x: 198, y: 430 },
      { x: 1052, y: 430 },
      { x: 226, y: 430 },
      { x: 1024, y: 430 },
    ],
    theme: HILL,
  },
  {
    id: 'wellsworth',
    name: 'Wellsworth Run',
    region: 'Main line',
    briefing: 'Longer water, same tools. If a beam turns red in the preview, it is too ambitious.',
    tools: ['road', 'wood'],
    parCost: 1760,
    budget: 4400,
    width: 1400,
    height: 680,
    waterY: 508,
    wind: 0,
    terrain: banks(250, 1040, 400, 1400, 680),
    start: { x: 228, y: 390 },
    end: { x: 1062, y: 390 },
    anchors: [
      { x: 228, y: 390 },
      { x: 1062, y: 390 },
      { x: 176, y: 390 },
      { x: 1114, y: 390 },
      { x: 200, y: 478 },
      { x: 1090, y: 478 },
    ],
    theme: HILL,
  },
  {
    id: 'brendam',
    name: 'Brendam Docks',
    region: 'Harbour branch',
    briefing: 'Steel is unlocked. Use it for long ties; keep the road for the crossing.',
    tools: ['road', 'wood', 'steel'],
    parCost: 2100,
    budget: 6400,
    width: 1460,
    height: 700,
    waterY: 520,
    wind: 12,
    terrain: banks(240, 1120, 410, 1460, 700),
    start: { x: 220, y: 400 },
    end: { x: 1140, y: 400 },
    anchors: [
      { x: 220, y: 400 },
      { x: 1140, y: 400 },
      { x: 168, y: 400 },
      { x: 1192, y: 400 },
      { x: 196, y: 492 },
      { x: 1164, y: 492 },
    ],
    theme: DOCK,
  },
  {
    id: 'vicarstown',
    name: 'Vicarstown Viaduct',
    region: 'Norramby line',
    briefing: 'A long industrial crossing. Mix steel hangers with a continuous road deck.',
    tools: ['road', 'wood', 'steel'],
    parCost: 2480,
    budget: 8000,
    width: 1540,
    height: 720,
    waterY: 540,
    wind: 18,
    terrain: banks(230, 1220, 418, 1540, 720),
    start: { x: 214, y: 406 },
    end: { x: 1236, y: 406 },
    anchors: [
      { x: 214, y: 406 },
      { x: 1236, y: 406 },
      { x: 168, y: 406 },
      { x: 1284, y: 406 },
      { x: 188, y: 510 },
      { x: 1262, y: 510 },
    ],
    theme: VIADUCT,
  },
  {
    id: 'ffarquhar',
    name: 'Ffarquhar Split',
    region: 'Quarry line',
    briefing: 'An island in the water. Treat it as two crossings, or one long deck with a pier.',
    tools: ['road', 'wood', 'steel'],
    parCost: 2360,
    budget: 8000,
    width: 1580,
    height: 720,
    waterY: 545,
    wind: 10,
    terrain: [
      { x0: -80, x1: 250, y: 418 },
      { x0: 700, x1: 880, y: 400 },
      { x0: 1320, x1: 1700, y: 418 },
    ],
    start: { x: 228, y: 408 },
    end: { x: 1350, y: 408 },
    anchors: [
      { x: 228, y: 408 },
      { x: 1350, y: 408 },
      { x: 176, y: 408 },
      { x: 1400, y: 408 },
      { x: 760, y: 390 },
      { x: 820, y: 390 },
      { x: 200, y: 500 },
      { x: 1372, y: 500 },
    ],
    theme: HILL,
  },
  {
    id: 'express',
    name: 'The Express',
    region: 'Main line — prestige',
    briefing: 'The longest gap, with wind. Gordon will test this later. Build like you mean it.',
    tools: ['road', 'wood', 'steel'],
    parCost: 2860,
    budget: 9000,
    width: 1680,
    height: 740,
    waterY: 560,
    wind: 28,
    terrain: banks(220, 1380, 430, 1680, 740),
    start: { x: 204, y: 418 },
    end: { x: 1408, y: 418 },
    anchors: [
      { x: 204, y: 418 },
      { x: 1408, y: 418 },
      { x: 156, y: 418 },
      { x: 1456, y: 418 },
      { x: 180, y: 520 },
      { x: 1432, y: 520 },
    ],
    theme: STORM,
  },
];

export function levelById(id: string): LevelDef {
  const found = LEVELS.find((level) => level.id === id);
  if (!found) {
    throw new Error(`Unknown level ${id}`);
  }
  return found;
}

export function levelIndex(id: string): number {
  return LEVELS.findIndex((level) => level.id === id);
}
