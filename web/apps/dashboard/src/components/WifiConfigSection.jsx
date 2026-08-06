/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

import { useEffect, useState } from 'preact/hooks';
import { ConfirmDialog } from './confirmation';

const CHANNELS = Array.from({ length: 13 }, (_, i) => i + 1);
const MIN_PASSWORD_LENGTH = 8;

function WifiConfigForm({ config, busy, setBusy, onChange }) {
    const [draft, setDraft] = useState(config);
    const [password, setPassword] = useState('');
    const [open, setOpen] = useState(!config['password-set']);
    const [confirmOpen, setConfirmOpen] = useState(false);

    useEffect(() => {
        setDraft(config);
        setPassword('');
        setOpen(!config['password-set']);
    }, [config]);

    // Empty means "leave the stored password alone", so it is only a valid
    // submission when there is already one stored.
    const passwordRequired = !open && !config['password-set'];

    async function save(e) {
        e.preventDefault();
        setBusy(true);
        try {
            const body = { ssid: draft.ssid, channel: draft.channel };
            if (open) body.password = '';
            else if (password !== '') body.password = password;

            const res = await fetch('/api/config/wifi', {
                method: 'PUT',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(body),
            });
            onChange(await res.json());
        } finally {
            setBusy(false);
        }
    }

    async function reset() {
        setBusy(true);
        try {
            const res = await fetch('/api/config/wifi', { method: 'DELETE' });
            onChange(await res.json());
        } finally {
            setBusy(false);
            setConfirmOpen(false);
        }
    }

    return (
        <form onSubmit={save}>
            <label for="wifi-ssid">
                Network name (SSID)
                <input
                    id="wifi-ssid"
                    type="text"
                    value={draft.ssid}
                    maxLength={32}
                    required
                    onInput={(e) =>
                        setDraft({ ...draft, ssid: e.target.value })
                    }
                />
                <small>
                    The dashboard stays reachable at http://teslasynth.local
                    whatever you name the network.
                </small>
            </label>

            <label for="wifi-password">
                Password
                <input
                    id="wifi-password"
                    type="password"
                    value={password}
                    minLength={MIN_PASSWORD_LENGTH}
                    maxLength={63}
                    disabled={open}
                    required={passwordRequired}
                    placeholder={
                        config['password-set']
                            ? 'Leave empty to keep the current password'
                            : 'At least 8 characters'
                    }
                    onInput={(e) => setPassword(e.target.value)}
                />
                <small>
                    Leave it empty unless you are changing it.
                </small>
            </label>

            <label>
                <input
                    type="checkbox"
                    checked={open}
                    onChange={(e) => setOpen(e.target.checked)}
                />
                Open network (no password)
            </label>
            {open && (
                <small>
                    Anyone in range will be able to reach the dashboard and
                    reconfigure the device while it is in maintenance mode.
                </small>
            )}

            <label for="wifi-channel">
                Channel
                <select
                    id="wifi-channel"
                    value={draft.channel}
                    onChange={(e) =>
                        setDraft({ ...draft, channel: Number(e.target.value) })
                    }
                >
                    {CHANNELS.map((c) => (
                        <option key={c} value={c}>
                            {c}
                        </option>
                    ))}
                </select>
            </label>

            <footer>
                <div class="grid">
                    <button type="submit" disabled={busy}>
                        Save
                    </button>
                    <button
                        type="button"
                        disabled={busy}
                        onClick={() => setConfirmOpen(true)}
                    >
                        Reset
                    </button>
                </div>
            </footer>

            <ConfirmDialog
                open={confirmOpen}
                title="Reset Wi-Fi settings"
                message="This restores the factory network name, password and channel. Continue?"
                busy={busy}
                onCancel={() => setConfirmOpen(false)}
                onConfirm={reset}
            />
        </form>
    );
}

export function WifiConfigSection() {
    const [cfg, setCfg] = useState(null);
    const [busy, setBusy] = useState(false);

    useEffect(() => {
        fetch('/api/config/wifi')
            .then((r) => r.json())
            .then(setCfg);
    }, []);

    if (!cfg) {
        return (
            <article>
                <header aria-busy="true">Loading Wi-Fi Configuration…</header>
            </article>
        );
    }

    return (
        <article>
            <header>
                <hgroup>
                    <h2>Wi-Fi Configuration</h2>
                    <p>Maintenance mode access point</p>
                </hgroup>
            </header>

            <WifiConfigForm
                config={cfg}
                busy={busy}
                setBusy={setBusy}
                onChange={setCfg}
            />

            <footer>
                <small>
                    Changes take effect the next time the device enters
                    maintenance mode. The current connection is left alone, so
                    you can keep configuring until you reboot.
                </small>
            </footer>
        </article>
    );
}
