/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

import { defineConfig } from 'vite';
import preact from '@preact/preset-vite';
import { viteSingleFile } from 'vite-plugin-singlefile';
import { mockApi } from './dev-mocks.js';

export default defineConfig({
    plugins: [preact(), viteSingleFile(), mockApi()],
    build: {
        target: 'es2017',
        minify: 'esbuild',
        cssMinify: true,
    },
});
