/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

export function FlashControls({ onFlash, busy }) {
    return (
        <button type="button" aria-busy={busy} onClick={onFlash}>
            Flash Device
        </button>
    );
}
