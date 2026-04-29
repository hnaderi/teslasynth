/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * The engine's heavy machinery (AudioContext, AudioWorklet, Web Worker, SAB
 * ring buffer) only spins up when play() is called. These tests exercise the
 * public API contract — bounds checking, mute state, program overrides, and
 * dispose — without ever entering the playback path, so no browser globals
 * are needed.
 */

import { describe, it, expect, beforeEach } from 'vitest';
import { createMidiEngine } from './midiEngine';

function makeStubFile(overrides = {}) {
    return {
        format: 0,
        nTracks: 1,
        ticksPerQN: 480,
        trackNames: ['Track 1'],
        trackChannels: [[0]],
        initBPM: 120,
        endUs: 1_000_000,
        events: [
            {
                type: 'midi',
                tick: 0,
                status: 0x90,
                ch: 0,
                b1: 60,
                b2: 100,
                msgType: 0x90,
                us: 0,
            },
            {
                type: 'midi',
                tick: 480,
                status: 0x80,
                ch: 0,
                b1: 60,
                b2: 0,
                msgType: 0x80,
                us: 1_000_000,
            },
        ],
        tempoEvents: [],
        ...overrides,
    };
}

function makeStubMidiOut() {
    const sent = [];
    return {
        sent,
        send: (msg) => sent.push([...msg]),
    };
}

describe('createMidiEngine', () => {
    let engine;
    let events;
    beforeEach(() => {
        events = [];
        engine = createMidiEngine({
            onEvent: (type, data) => events.push({ type, data }),
            onActivity: () => {},
        });
    });

    it('emits a "load" event when a file is loaded', () => {
        const file = makeStubFile();
        engine.load(file);
        const load = events.find((e) => e.type === 'load');
        expect(load).toBeDefined();
        expect(load.data).toBe(file);
    });

    it('getPositionUs returns 0 before play', () => {
        engine.load(makeStubFile());
        expect(engine.getPositionUs()).toBe(0);
        expect(engine.isPlayingNow()).toBe(false);
    });

    describe('setVolume clamps to [0, 127] and broadcasts on every channel', () => {
        it.each([
            [200, 127],
            [-50, 0],
            [64, 64],
        ])('setVolume(%i) → sends %i', (input, expected) => {
            const port = makeStubMidiOut();
            engine.setMidiOut(port);
            engine.setVolume(input);
            // 16 channels × 1 CC each
            expect(port.sent).toHaveLength(16);
            for (const msg of port.sent) {
                expect(msg[0] & 0xf0).toBe(0xb0); // Control Change
                expect(msg[1]).toBe(7); // Main Volume CC
                expect(msg[2]).toBe(expected);
            }
        });
    });

    describe('setTempoScale and setTranspose clamp to safe ranges', () => {
        // We can't peek at the closure directly, but we can verify that calling
        // these with extreme values doesn't throw and doesn't desynchronise the
        // engine state (isPlayingNow stays false, getPositionUs stays 0).
        it.each([
            ['setTempoScale', 10],
            ['setTempoScale', 0.01],
            ['setTranspose', 50],
            ['setTranspose', -50],
        ])('%s(%f) is silent and stable', (method, value) => {
            engine.load(makeStubFile());
            expect(() => engine[method](value)).not.toThrow();
            expect(engine.isPlayingNow()).toBe(false);
            expect(engine.getPositionUs()).toBe(0);
        });
    });

    it('muteTrack toggles a track in and out of the muted set without throwing', () => {
        engine.load(makeStubFile());
        expect(() => engine.muteTrack(0, true)).not.toThrow();
        expect(() => engine.muteTrack(0, false)).not.toThrow();
        // Toggling many times is harmless.
        for (let i = 0; i < 5; i++) engine.muteTrack(0, i % 2 === 0);
    });

    describe('setChannelProgram', () => {
        it('sends Program Change immediately when assigned', () => {
            const port = makeStubMidiOut();
            engine.setMidiOut(port);
            engine.setChannelProgram(3, 42);
            expect(port.sent).toContainEqual([0xc0 | 3, 42]);
        });

        it('removes the override silently when program is negative', () => {
            const port = makeStubMidiOut();
            engine.setMidiOut(port);
            engine.setChannelProgram(3, 42);
            port.sent.length = 0;
            // Negative program clears the override; no message sent.
            expect(() => engine.setChannelProgram(3, -1)).not.toThrow();
            expect(port.sent).toHaveLength(0);
        });

        it('does not throw when no MIDI output is connected', () => {
            expect(() => engine.setChannelProgram(0, 5)).not.toThrow();
        });
    });

    it('seek(us) before play parks the offset within [0, endUs]', () => {
        engine.load(makeStubFile({ endUs: 1_000_000 }));
        engine.seek(500_000);
        expect(engine.isPlayingNow()).toBe(false);
        expect(engine.getPositionUs()).toBe(500_000);

        engine.seek(2_000_000); // beyond end → clamps to endUs
        expect(engine.getPositionUs()).toBe(1_000_000);

        engine.seek(-100); // before start → clamps to 0
        expect(engine.getPositionUs()).toBe(0);
    });

    it('allNotesOff is a no-op when no MIDI output is set', () => {
        expect(() => engine.allNotesOff()).not.toThrow();
    });

    it('allNotesOff broadcasts 123 (all-notes-off) and 120 (all-sound-off) on every channel', () => {
        const port = makeStubMidiOut();
        engine.setMidiOut(port);
        engine.allNotesOff();
        expect(port.sent).toHaveLength(32); // 16 channels × 2 messages
        const ccs = port.sent.map((m) => m[1]);
        expect(ccs.filter((c) => c === 123)).toHaveLength(16);
        expect(ccs.filter((c) => c === 120)).toHaveLength(16);
    });

    it('setMidiOut(null) detaches the port', () => {
        const port = makeStubMidiOut();
        engine.setMidiOut(port);
        engine.setMidiOut(null);
        engine.allNotesOff();
        expect(port.sent).toHaveLength(0);
    });

    it('dispose is safe to call when nothing has been started', () => {
        expect(() => engine.dispose()).not.toThrow();
    });

    it('setLoop toggles the loop flag without throwing', () => {
        expect(() => engine.setLoop(true)).not.toThrow();
        expect(() => engine.setLoop(false)).not.toThrow();
    });
});
