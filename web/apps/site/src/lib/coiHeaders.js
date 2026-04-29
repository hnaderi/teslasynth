/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Wraps a Response with the COOP/COEP header pair required for
 * `crossOriginIsolated` (and thus SharedArrayBuffer) on static hosts that can't
 * set these headers themselves. Used by the service worker for navigation.
 */

export function withCoi(response) {
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
