/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Shared plumbing for the configuration forms. Each one edits a single JSON
 * document behind GET/PUT/DELETE, and all of them behave identically.
 *
 * Nothing is ever pushed to the device implicitly: the form cancels submit
 * events, and Save is a plain button, so pressing Enter in a field or hitting
 * a stray button cannot apply a half-edited configuration.
 */

import { useState } from 'preact/hooks';
import { ConfirmDialog } from './confirmation';

// Errors come back as plain text from the firmware, not JSON.
async function failure(res) {
    const body = await res.text().catch(() => '');
    return `${res.status} ${body.trim() || res.statusText}`;
}

export function useConfigEndpoint(url, setBusy, onUpdate) {
    const [error, setError] = useState(null);

    async function request(init) {
        setBusy(true);
        setError(null);
        try {
            const res = await fetch(url, init);
            if (!res.ok) {
                setError(await failure(res));
                return false;
            }
            onUpdate(await res.json());
            return true;
        } catch (err) {
            setError(String(err));
            return false;
        } finally {
            setBusy(false);
        }
    }

    return {
        error,
        save: (body) =>
            request({
                method: 'PUT',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(body),
            }),
        reset: () => request({ method: 'DELETE' }),
    };
}

export function ConfigForm({ children }) {
    return <form onSubmit={(e) => e.preventDefault()}>{children}</form>;
}

export function ConfigFormActions({
    busy,
    error,
    onSave,
    onReset,
    resetTitle,
    resetMessage,
    children,
}) {
    const [confirmOpen, setConfirmOpen] = useState(false);

    // Submit is blocked, so constraint validation has to be asked for.
    function save(e) {
        const form = e.currentTarget.form;
        if (form && !form.reportValidity()) return;
        onSave();
    }

    async function reset() {
        await onReset();
        setConfirmOpen(false);
    }

    return (
        <footer>
            {error && (
                <p role="alert">
                    <small>Could not save configuration: {error}</small>
                </p>
            )}

            <div class="grid">
                <button type="button" onClick={save} disabled={busy}>
                    Save
                </button>
                <button
                    type="button"
                    onClick={() => setConfirmOpen(true)}
                    disabled={busy}
                >
                    Reset
                </button>
                {children}
            </div>

            <ConfirmDialog
                open={confirmOpen}
                title={resetTitle}
                message={resetMessage}
                busy={busy}
                onCancel={() => setConfirmOpen(false)}
                onConfirm={reset}
            />
        </footer>
    );
}
