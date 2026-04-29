/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

// Stub the esp-web-tools side-effect import so this test runs in plain Node.
import { vi } from 'vitest';
vi.mock('esp-web-tools/dist/install-button.js', () => ({}));

import { describe, it, expect } from 'vitest';
import { toEspWebToolsManifest } from './InstallButton';

const sampleManifest = {
    name: 'Teslasynth',
    targets: {
        esp32: {
            extra_esptool_args: { chip: 'esp32' },
            files: [{ path: 'esp32/firmware.bin', offset: '0x10000' }],
        },
        esp32s2: {
            extra_esptool_args: { chip: 'esp32s2' },
            files: [
                { path: 'esp32s2/bootloader.bin', offset: '0x1000' },
                { path: 'esp32s2/firmware.bin', offset: '0x10000' },
            ],
        },
    },
};

describe('toEspWebToolsManifest', () => {
    it('maps known chip ids to esp-web-tools chipFamily strings', () => {
        const out = toEspWebToolsManifest(sampleManifest, '1.0.0');
        const chips = out.builds.map((b) => b.chipFamily).sort();
        expect(chips).toEqual(['ESP32', 'ESP32-S2']);
    });

    it('falls through to the raw chip value when not in CHIP_FAMILY', () => {
        const m = {
            name: 'x',
            targets: {
                custom: {
                    extra_esptool_args: { chip: 'esp32-future' },
                    files: [{ path: 'a.bin', offset: '0x0' }],
                },
            },
        };
        const out = toEspWebToolsManifest(m, '1.0.0');
        expect(out.builds[0].chipFamily).toBe('esp32-future');
    });

    it('builds absolute file paths from the CDN base + version', () => {
        const out = toEspWebToolsManifest(sampleManifest, '2.3.4');
        const paths = out.builds.flatMap((b) => b.parts.map((p) => p.path));
        expect(paths.every((p) => p.includes('/teslasynth@firmware/2.3.4/'))).toBe(true);
        expect(paths).toContain(
            'https://cdn.jsdelivr.net/gh/hnaderi/teslasynth@firmware/2.3.4/esp32s2/bootloader.bin'
        );
    });

    it('parses hex offsets to integers', () => {
        const out = toEspWebToolsManifest(sampleManifest, '1.0.0');
        const offsets = out.builds.flatMap((b) => b.parts.map((p) => p.offset));
        expect(offsets).toContain(0x10000);
        expect(offsets).toContain(0x1000);
        expect(offsets.every((o) => typeof o === 'number' && !Number.isNaN(o))).toBe(true);
    });

    it('sets new_install_prompt_erase: true', () => {
        const out = toEspWebToolsManifest(sampleManifest, '1.0.0');
        expect(out.new_install_prompt_erase).toBe(true);
    });
});
