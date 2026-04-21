/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

export function BooleanIndicator({ value, label }) {
    return (
        <span
            role="status"
            aria-label={`${label}: ${value ? 'available' : 'not available'}`}
            class={value ? 'bool-yes' : 'bool-no'}
        >
            {value ? '✓' : '✕'}
        </span>
    );
}
