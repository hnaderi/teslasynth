/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

import { defineConfig } from 'vite';
import preact from '@preact/preset-vite';
import mdx from '@mdx-js/rollup';
import remarkGfm from 'remark-gfm';
import rehypePrettyCode from 'rehype-pretty-code';

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
    ],
    server: {
        fs: { allow: ['../../../..'] }, // allow reading firmware source files
    },
    build: {
        target: 'es2017',
        minify: 'esbuild',
        cssMinify: true,
    },
});
