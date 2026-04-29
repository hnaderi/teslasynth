/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

// Stub the ?raw firmware header import so this test runs without the file.
import { vi } from 'vitest';
vi.mock('../../../../../lib/synthesizer/bank/instruments.hpp?raw', () => ({
    default: '',
}));

import { describe, it, expect } from 'vitest';
import { parseInstruments } from './instruments';

describe('parseInstruments', () => {
    it('extracts quoted names from the marker block', () => {
        const src = `
            // ... unrelated header content ...
            constexpr auto instrument_names = {{
                "Sine",
                "Square",
                "Sawtooth",
            }};
        `;
        expect(parseInstruments(src)).toEqual(['Sine', 'Square', 'Sawtooth']);
    });

    it('returns an empty array when the marker is absent', () => {
        expect(parseInstruments('no marker in this file')).toEqual([]);
    });

    it('ignores quoted strings outside the marker block', () => {
        const src = `
            const char* greeting = "Hello, world";
            constexpr auto instrument_names = {{
                "OnlyThis",
            }};
            const char* footer = "Goodbye";
        `;
        expect(parseInstruments(src)).toEqual(['OnlyThis']);
    });
});
