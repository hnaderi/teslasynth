/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

import { describe, it, expect } from 'vitest';
import { parseMidi } from './midiParser';
import {
    buildMidi,
    header,
    track,
    smpteHeader,
    proprietaryChunk,
} from '../../test/midiFixtures';

describe('parseMidi', () => {
    it('parses a minimal format-0 file with one note', () => {
        const buf = buildMidi(
            header({ format: 0, nTracks: 1, ticksPerQN: 480 }),
            track([
                { dt: 0, type: 'noteOn', ch: 0, note: 60, vel: 100 },
                { dt: 480, type: 'noteOff', ch: 0, note: 60 },
            ])
        );
        const r = parseMidi(buf);
        expect(r.format).toBe(0);
        expect(r.nTracks).toBe(1);
        expect(r.ticksPerQN).toBe(480);
        const midi = r.events.filter((e) => e.type === 'midi');
        expect(midi).toHaveLength(2);
        expect(midi[0]).toMatchObject({
            ch: 0,
            b1: 60,
            b2: 100,
            msgType: 0x90,
        });
        expect(midi[1]).toMatchObject({ ch: 0, b1: 60, msgType: 0x80 });
    });

    it('throws on a non-MIDI buffer', () => {
        const buf = new Uint8Array([1, 2, 3, 4, 5, 6, 7, 8]).buffer;
        expect(() => parseMidi(buf)).toThrow(/Not a MIDI file/);
    });

    it('rejects SMPTE time division', () => {
        const buf = buildMidi(smpteHeader());
        expect(() => parseMidi(buf)).toThrow(/SMPTE/);
    });

    it('decodes variable-length quantities of multiple sizes', () => {
        // 1-byte (dt=0x40=64), 2-byte (dt=0x81 0x00 = 128), 3-byte (dt=0xC0 0x80 0x00 = 1048576)
        const buf = buildMidi(
            header({ nTracks: 1 }),
            track([
                { dt: 0x40, type: 'noteOn', ch: 0, note: 60 },
                { dt: 128, type: 'noteOn', ch: 0, note: 61 },
                { dt: 1048576, type: 'noteOn', ch: 0, note: 62 },
            ])
        );
        const r = parseMidi(buf);
        const ticks = r.events
            .filter((e) => e.type === 'midi')
            .map((e) => e.tick);
        expect(ticks).toEqual([0x40, 0x40 + 128, 0x40 + 128 + 1048576]);
    });

    it('applies tempo changes to the µs timeline', () => {
        // 480 ticks/qn. Default tempo 500000 µs/qn until tick 480, then 1000000 µs/qn.
        // Note at tick 0 → 0 µs.  Note at tick 480 → 500000 µs.
        // Note at tick 960 → 500000 + 480 * (1000000/480) = 1500000 µs.
        const buf = buildMidi(
            header({ nTracks: 1, ticksPerQN: 480 }),
            track([
                { dt: 0, type: 'noteOn', ch: 0, note: 60 },
                { dt: 480, type: 'tempo', uspq: 1000000 },
                { dt: 0, type: 'noteOn', ch: 0, note: 61 },
                { dt: 480, type: 'noteOn', ch: 0, note: 62 },
            ])
        );
        const r = parseMidi(buf);
        const notes = r.events.filter((e) => e.type === 'midi');
        expect(notes[0].us).toBeCloseTo(0, 3);
        expect(notes[1].us).toBeCloseTo(500000, 3);
        expect(notes[2].us).toBeCloseTo(1500000, 3);
    });

    it('honours running status (omitted status byte)', () => {
        // Two consecutive note-ons sharing the 0x90 status. Second event has no status byte.
        const buf = buildMidi(
            header({ nTracks: 1 }),
            track([
                { dt: 0, type: 'noteOn', ch: 0, note: 60, vel: 100 },
                // Same status implied — emit only the data bytes.
                { dt: 10, type: 'data', bytes: [62, 90] },
            ])
        );
        const r = parseMidi(buf);
        const notes = r.events.filter((e) => e.type === 'midi');
        expect(notes).toHaveLength(2);
        expect(notes[1]).toMatchObject({ status: 0x90, b1: 62, b2: 90 });
    });

    it('skips unknown chunks between tracks (DAW compatibility)', () => {
        const buf = buildMidi(
            header({ nTracks: 1 }),
            proprietaryChunk([0xde, 0xad, 0xbe, 0xef]),
            track([{ dt: 0, type: 'noteOn', ch: 0, note: 60 }])
        );
        const r = parseMidi(buf);
        expect(r.events.filter((e) => e.type === 'midi')).toHaveLength(1);
    });

    it('extracts track names from meta events and drops null bytes', () => {
        const buf = buildMidi(
            header({ nTracks: 1 }),
            track([
                { dt: 0, type: 'trackName', name: 'Lead\0\0' },
                { dt: 0, type: 'noteOn', ch: 0, note: 60 },
            ])
        );
        const r = parseMidi(buf);
        expect(r.trackNames).toEqual(['Lead']);
    });

    it('falls back to "Track N" when no name event is present', () => {
        const buf = buildMidi(
            header({ nTracks: 2 }),
            track([{ dt: 0, type: 'noteOn', ch: 0, note: 60 }]),
            track([{ dt: 0, type: 'noteOn', ch: 1, note: 64 }])
        );
        const r = parseMidi(buf);
        expect(r.trackNames).toEqual(['Track 1', 'Track 2']);
    });

    it('computes initBPM from the first tempo event', () => {
        // 250000 µs/qn = 240 BPM
        const buf = buildMidi(
            header({ nTracks: 1 }),
            track([
                { dt: 0, type: 'tempo', uspq: 250000 },
                { dt: 0, type: 'noteOn', ch: 0, note: 60 },
            ])
        );
        expect(parseMidi(buf).initBPM).toBe(240);
    });

    it('defaults initBPM to 120 when no tempo event is present', () => {
        const buf = buildMidi(
            header({ nTracks: 1 }),
            track([{ dt: 0, type: 'noteOn', ch: 0, note: 60 }])
        );
        expect(parseMidi(buf).initBPM).toBe(120);
    });

    it('places tempo events before midi events at the same tick', () => {
        // If a tempo change shares a tick with note-ons in another track, the tempo
        // must apply BEFORE the µs of those notes is computed.
        const buf = buildMidi(
            header({ format: 1, nTracks: 2 }),
            track([{ dt: 480, type: 'tempo', uspq: 1000000 }]),
            track([
                { dt: 0, type: 'noteOn', ch: 0, note: 60 },
                { dt: 480, type: 'noteOn', ch: 0, note: 61 },
                // Note in second track at the same tick as the tempo change.
                { dt: 480, type: 'noteOn', ch: 0, note: 62 },
            ])
        );
        const r = parseMidi(buf);
        const tempoEv = r.events.find((e) => e.type === 'tempo');
        const noteAtSameTick = r.events
            .filter((e) => e.type === 'midi' && e.tick === tempoEv.tick)
            .pop();
        // Tempo µs computed under old rate (500k), but the next gap should use the new rate.
        // Verify ordering: tempo appears before any midi event at that tick in the merged array.
        const tempoIdx = r.events.indexOf(tempoEv);
        const midiAtTick = r.events.findIndex(
            (e) => e.type === 'midi' && e.tick === tempoEv.tick
        );
        expect(tempoIdx).toBeLessThan(midiAtTick);
        expect(noteAtSameTick).toBeDefined();
    });

    it('collects the unique channels used in each track', () => {
        const buf = buildMidi(
            header({ format: 1, nTracks: 2 }),
            track([
                { dt: 0, type: 'noteOn', ch: 0, note: 60 },
                { dt: 0, type: 'noteOn', ch: 0, note: 61 },
                { dt: 0, type: 'noteOn', ch: 2, note: 62 },
            ]),
            track([{ dt: 0, type: 'noteOn', ch: 9, note: 36 }])
        );
        const r = parseMidi(buf);
        expect(r.trackChannels[0].sort()).toEqual([0, 2]);
        expect(r.trackChannels[1]).toEqual([9]);
    });

    it('endUs equals the largest event µs', () => {
        const buf = buildMidi(
            header({ nTracks: 1, ticksPerQN: 480 }),
            track([
                { dt: 0, type: 'noteOn', ch: 0, note: 60 },
                { dt: 480, type: 'noteOff', ch: 0, note: 60 },
                { dt: 480, type: 'noteOn', ch: 0, note: 64 },
            ])
        );
        const r = parseMidi(buf);
        const lastUs = Math.max(...r.events.map((e) => e.us));
        expect(r.endUs).toBe(lastUs);
        expect(r.endUs).toBeCloseTo(1000000, 3); // 2 quarter notes at 120 BPM = 1s
    });
});
