import { defineConfig } from 'vite';
import preact from '@preact/preset-vite';
import { viteSingleFile } from 'vite-plugin-singlefile';

export default defineConfig({
    plugins: [
        preact(),
        viteSingleFile(),
    ],
    build: {
        target: 'es2017',
        minify: 'esbuild',
        cssMinify: true,
    },
});
