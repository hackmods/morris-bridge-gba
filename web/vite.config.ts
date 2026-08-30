import { resolve } from 'node:path';
import { defineConfig } from 'vite';

const crossOriginIsolationHeaders = {
  'Cross-Origin-Opener-Policy': 'same-origin',
  'Cross-Origin-Embedder-Policy': 'require-corp',
};

export default defineConfig({
  server: {
    port: 4317,
    host: '127.0.0.1',
    headers: crossOriginIsolationHeaders,
  },
  preview: {
    port: 4317,
    host: '127.0.0.1',
    headers: crossOriginIsolationHeaders,
  },
  optimizeDeps: {
    exclude: ['@thenick775/mgba-wasm'],
  },
  build: {
    rollupOptions: {
      input: {
        main: resolve(__dirname, 'index.html'),
        legacy: resolve(__dirname, 'legacy.html'),
      },
    },
  },
});
