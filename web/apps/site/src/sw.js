/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

/**
 * Service worker: precache all build assets + COOP/COEP headers for SAB.
 * Manifest injected by vite-plugin-pwa; cache lifecycle by workbox-precaching.
 */

import { PrecacheController } from 'workbox-precaching';
import { withCoi } from './lib/coiHeaders';

const precache = new PrecacheController({ cacheName: 'teslasynth' });
precache.addToCacheList(self.__WB_MANIFEST);

self.addEventListener('install', (e) => {
    e.waitUntil(precache.install(e).then(() => self.skipWaiting()));
});

self.addEventListener('activate', (e) => {
    e.waitUntil(precache.activate(e).then(() => self.clients.claim()));
});

self.addEventListener('fetch', (e) => {
    if (
        e.request.cache === 'only-if-cached' &&
        e.request.mode !== 'same-origin'
    )
        return;

    const url = new URL(e.request.url);
    if (url.origin !== self.location.origin) return;

    const isNavigation = e.request.mode === 'navigate';

    // Only intercept navigation and JS/CSS/WASM — everything else (manifest,
    // favicon, icons) flows through to the browser without SW-initiated fetches.
    // COI headers are only required on the HTML document, not same-origin subresources.
    if (!isNavigation && !/\.(js|css|wasm)(\?|$)/.test(url.pathname)) return;

    e.respondWith(
        (async () => {
            const cacheKey = isNavigation
                ? precache.getCacheKeyForURL('/index.html')
                : precache.getCacheKeyForURL(url.pathname);

            if (cacheKey) {
                const cached = await caches.match(cacheKey, {
                    cacheName: 'teslasynth',
                });
                if (cached) return isNavigation ? withCoi(cached) : cached;
            }

            try {
                const req = isNavigation
                    ? new Request('/index.html')
                    : e.request;
                const response = await fetch(req);
                return isNavigation ? withCoi(response) : response;
            } catch {
                return new Response('Offline', { status: 503 });
            }
        })()
    );
});
