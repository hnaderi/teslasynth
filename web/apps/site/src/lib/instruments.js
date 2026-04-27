/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Instrument names are parsed at build time from the firmware header so this
 * list is always in sync without manual maintenance.
 */
import raw from '../../../../../lib/synthesizer/bank/instruments.hpp?raw';

function parseInstruments(src) {
    const start = src.indexOf('instrument_names = {{');
    const end = src.indexOf('}}', start);
    return [...src.slice(start, end).matchAll(/"([^"]+)"/g)].map((m) => m[1]);
}

export const INSTRUMENTS = parseInstruments(raw);
