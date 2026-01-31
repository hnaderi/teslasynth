import { defineConfig } from 'vite';
import preact from '@preact/preset-vite';

export default defineConfig({
    base: "./",
    plugins: [
        preact(),
    ],
    build: {
        target: 'es2017',
        minify: 'esbuild',
        cssMinify: true,
    },
});
