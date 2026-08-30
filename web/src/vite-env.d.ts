/// <reference types="vite/client" />

import type { GameHooks } from './game/game';

declare global {
  interface Window {
    __GAME__?: GameHooks;
  }
}

export {};
