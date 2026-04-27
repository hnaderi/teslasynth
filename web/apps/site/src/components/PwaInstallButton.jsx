/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

import { useState, useEffect } from 'preact/hooks';

export function PwaInstallButton() {
    const [prompt, setPrompt] = useState(null);

    useEffect(() => {
        function onPrompt(e) {
            e.preventDefault();
            setPrompt(e);
        }
        window.addEventListener('beforeinstallprompt', onPrompt);
        return () =>
            window.removeEventListener('beforeinstallprompt', onPrompt);
    }, []);

    if (!prompt) return null;

    async function install() {
        prompt.prompt();
        const { outcome } = await prompt.userChoice;
        if (outcome === 'accepted') setPrompt(null);
    }

    return (
        <button onClick={install} class="pwa-install-btn outline secondary">
            Install app
        </button>
    );
}
