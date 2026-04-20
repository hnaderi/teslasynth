import { defineConfig } from 'vite';
import preact from '@preact/preset-vite';
import mdx from '@mdx-js/rollup';

export default defineConfig({
    base: "/teslasynth",
    plugins: [
        { enforce: 'pre', ...mdx({ jsxImportSource: 'preact', providerImportSource: '@mdx-js/preact' }) },
        preact(),
    ],
    build: {
        target: 'es2017',
        minify: 'esbuild',
        cssMinify: true,
    },
});
