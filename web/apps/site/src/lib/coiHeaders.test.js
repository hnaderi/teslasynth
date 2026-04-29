/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

import { describe, it, expect } from 'vitest';
import { withCoi } from './coiHeaders';

describe('withCoi', () => {
    it('adds COOP same-origin and COEP require-corp', () => {
        const r = withCoi(new Response('hi', { status: 200 }));
        expect(r.headers.get('Cross-Origin-Opener-Policy')).toBe('same-origin');
        expect(r.headers.get('Cross-Origin-Embedder-Policy')).toBe('require-corp');
    });

    it('preserves status, statusText, body, and other headers', async () => {
        const original = new Response('payload', {
            status: 201,
            statusText: 'Created',
            headers: { 'X-Custom': 'keep-me' },
        });
        const r = withCoi(original);
        expect(r.status).toBe(201);
        expect(r.statusText).toBe('Created');
        expect(r.headers.get('X-Custom')).toBe('keep-me');
        expect(await r.text()).toBe('payload');
    });

    it('returns the response unchanged when status is 0 (opaque)', () => {
        // Real Response can't be constructed with status 0 in non-browser envs;
        // only opaque fetches receive one. Duck-type instead.
        const opaque = { status: 0, headers: new Headers() };
        expect(withCoi(opaque)).toBe(opaque);
    });

    it('returns null/undefined unchanged', () => {
        expect(withCoi(null)).toBeNull();
        expect(withCoi(undefined)).toBeUndefined();
    });
});
