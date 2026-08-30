export type MaterialId = 'road' | 'wood' | 'steel';

export interface Material {
  id: MaterialId;
  name: string;
  hint: string;
  costPerPx: number;
  stiffness: number;
  maxStrain: number;
  maxLength: number;
  minLength: number;
  traversable: boolean;
  color: string;
  colorDark: string;
  colorLight: string;
}

export const MATERIALS: Record<MaterialId, Material> = {
  road: {
    id: 'road',
    name: 'Road deck',
    hint: 'The train can only travel on road. Keep it reasonably short.',
    costPerPx: 2.2,
    stiffness: 0.62,
    maxStrain: 0.13,
    maxLength: 270,
    minLength: 28,
    traversable: true,
    color: '#3f3c3a',
    colorDark: '#2a3830',
    colorLight: '#d4c48a',
  },
  wood: {
    id: 'wood',
    name: 'Timber brace',
    hint: 'Cheap triangles. Great for holding the deck up. The train will not drive on it.',
    costPerPx: 1.05,
    stiffness: 0.48,
    maxStrain: 0.11,
    maxLength: 300,
    minLength: 24,
    traversable: false,
    color: '#c4894a',
    colorDark: '#8a5a28',
    colorLight: '#e2b57a',
  },
  steel: {
    id: 'steel',
    name: 'Steel rod',
    hint: 'Strong, expensive, and not a road. Use it when timber is not enough.',
    costPerPx: 3.15,
    stiffness: 0.78,
    maxStrain: 0.17,
    maxLength: 340,
    minLength: 24,
    traversable: false,
    color: '#7d8b99',
    colorDark: '#4c5864',
    colorLight: '#d5dee6',
  },
};

export const MATERIAL_ORDER: MaterialId[] = ['road', 'wood', 'steel'];
