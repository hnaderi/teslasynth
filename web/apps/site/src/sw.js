/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

/**
 * Service worker: precache all build assets + COOP/COEP headers for SAB.
 * Manifest injected by vite-plugin-pwa; cache lifecycle by workbox-precaching.
 */

import { PrecacheController } from 'workbox-precaching';

const precache = new PrecacheController({ cacheName: 'teslasynth' });
precache.addToCacheList(self.__WB_MANIFEST);

self.addEventListener('install', (e) => {
    e.waitUntil(precache.install(e).then(() => self.skipWaiting()));
});

self.addEventListener('activate', (e) => {
    e.waitUntil(precache.activate(e).then(() => self.clients.claim()));
});

function withCoi(response) {
    if (!response || response.status === 0) return response;
    const headers = new Headers(response.headers);
    headers.set('Cross-Origin-Opener-Policy', 'same-origin');
    headers.set('Cross-Origin-Embedder-Policy', 'require-corp');
    return new Response(response.body, {
        status: response.status,
        statusText: response.statusText,
        headers,
    });
}

self.addEventListener('fetch', (e) => {
    if (
        e.request.cache === 'only-if-cached' &&
        e.request.mode !== 'same-origin'
    )
        return;

    const url = new URL(e.request.url);
    if (url.origin !== self.location.origin) return;

    e.respondWith(
        (async () => {
            // Serve navigation (any URL) from the precached index.html
            const cacheKey =
                e.request.mode === 'navigate'
                    ? precache.getCacheKeyForURL('/index.html')
                    : precache.getCacheKeyForURL(url.pathname);

            if (cacheKey) {
                const cached = await caches.match(cacheKey, {
                    cacheName: 'teslasynth',
                });
                if (cached) return withCoi(cached);
            }

            // Runtime network fallback — cache for subsequent offline use
            try {
                const response = await fetch(e.request);
                if (response.ok) {
                    const cache = await caches.open('teslasynth-runtime');
                    cache.put(e.request, response.clone());
                }
                return withCoi(response);
            } catch {
                const runtime = await caches.open('teslasynth-runtime');
                const cached = await runtime.match(e.request);
                return (
                    withCoi(cached) ?? new Response('Offline', { status: 503 })
                );
            }
        })()
    );
});
