/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

// https://github.com/gzuidhof/coi-serviceworker
// Adds COOP + COEP headers so SharedArrayBuffer works on static hosts.
self.addEventListener('install', () => self.skipWaiting());
self.addEventListener('activate', (e) => e.waitUntil(self.clients.claim()));

self.addEventListener('fetch', (e) => {
    if (
        e.request.cache === 'only-if-cached' &&
        e.request.mode !== 'same-origin'
    )
        return;
    e.respondWith(
        fetch(e.request)
            .then((r) => {
                if (r.status === 0) return r;
                const h = new Headers(r.headers);
                h.set('Cross-Origin-Opener-Policy', 'same-origin');
                h.set('Cross-Origin-Embedder-Policy', 'require-corp');
                return new Response(r.body, {
                    status: r.status,
                    statusText: r.statusText,
                    headers: h,
                });
            })
            .catch(() => fetch(e.request))
    );
});
