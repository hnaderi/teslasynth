/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

import { useEffect, useState } from 'preact/hooks';

const SCOPES = [
    ['hardware', 'Hardware Configuration'],
    ['synth', 'Synth Configuration'],
];

export function StatusBanner() {
    const [status, setStatus] = useState(null);

    useEffect(() => {
        fetch('/api/sys/status')
            .then((r) => r.json())
            .then(setStatus)
            .catch(() => setStatus(null));
    }, []);

    if (!status || status.configured) return null;

    const missing = SCOPES.filter(([key]) => status.reasons?.[key]);

    return (
        <article>
            <header>
                <hgroup>
                    <h2>Configuration required</h2>
                    <p>
                        The device will not start normal operation until this is
                        resolved.
                    </p>
                </hgroup>
            </header>

            <ul>
                {missing.map(([key, label]) => (
                    <li key={key}>
                        <strong>{label}</strong>: {status.reasons[key]}
                    </li>
                ))}
            </ul>

            <footer>
                <small>
                    Fill in the section below and save. Outputs stay idle in
                    maintenance mode, so nothing is driven until you reboot.
                </small>
            </footer>
        </article>
    );
}
