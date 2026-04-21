/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

export const api = {
    getConfig: async () => {
        // const r = await fetch("/api/config");
        // return r.json();
        return { name: 'Name', ssid: 'Teslasynth' };
    },

    setConfig: async (data) => {
        const r = await fetch('/api/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(data),
        });
        return r.json();
    },
};
