/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

import { defineConfig } from 'vite';
import preact from '@preact/preset-vite';
import mdx from '@mdx-js/rollup';
import remarkGfm from 'remark-gfm';
import rehypePrettyCode from 'rehype-pretty-code';
import { VitePWA } from 'vite-plugin-pwa';

export default defineConfig({
    base: '/',
    plugins: [
        {
            enforce: 'pre',
            ...mdx({
                jsxImportSource: 'preact',
                providerImportSource: '@mdx-js/preact',
                remarkPlugins: [remarkGfm],
                rehypePlugins: [
                    [
                        rehypePrettyCode,
                        {
                            theme: {
                                light: 'github-light',
                                dark: 'github-dark-dimmed',
                            },
                        },
                    ],
                ],
            }),
        },
        preact(),
        VitePWA({
            strategies: 'injectManifest',
            srcDir: 'src',
            filename: 'sw.js',
            manifest: false, // we have our own manifest.webmanifest in public/
            injectRegister: null, // we register manually in index.html
        }),
    ],
    server: {
        fs: { allow: ['../../../..'] }, // allow reading firmware source files
        headers: {
            'Cross-Origin-Opener-Policy': 'same-origin',
            'Cross-Origin-Embedder-Policy': 'require-corp',
        },
    },
    build: {
        target: 'es2017',
        minify: 'esbuild',
        cssMinify: true,
    },
});
