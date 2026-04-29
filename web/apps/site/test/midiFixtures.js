/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Tiny SMF (Standard MIDI File) builder for tests. Lets us write parser
 * tests against in-memory buffers instead of binary fixtures on disk.
 */

function varlen(n) {
    const bytes = [n & 0x7f];
    n >>= 7;
    while (n > 0) {
        bytes.unshift((n & 0x7f) | 0x80);
        n >>= 7;
    }
    return bytes;
}

function chunk(magic, payload) {
    const out = [];
    for (const c of magic) out.push(c.charCodeAt(0));
    const len = payload.length;
    out.push((len >>> 24) & 0xff, (len >>> 16) & 0xff, (len >>> 8) & 0xff, len & 0xff);
    out.push(...payload);
    return out;
}

export function header({ format = 0, nTracks = 1, ticksPerQN = 480 } = {}) {
    return chunk('MThd', [
        (format >>> 8) & 0xff, format & 0xff,
        (nTracks >>> 8) & 0xff, nTracks & 0xff,
        (ticksPerQN >>> 8) & 0xff, ticksPerQN & 0xff,
    ]);
}

export function smpteHeader() {
    return chunk('MThd', [0, 0, 0, 1, 0xe7, 0x28]); // negative-frame SMPTE
}

/**
 * Build a track from a list of {dt, type, ...} events.
 * Supported types:
 *   { dt, type: 'noteOn',  ch, note, vel }
 *   { dt, type: 'noteOff', ch, note, vel }
 *   { dt, type: 'cc',      ch, ctrl, val }
 *   { dt, type: 'tempo',   uspq }
 *   { dt, type: 'trackName', name }
 *   { dt, type: 'instrument', name }
 *   { dt, type: 'rawStatus', byte }   // emit a raw status (useful for running-status tests)
 *   { dt, type: 'data',    bytes: [...] }
 */
export function track(events) {
    const out = [];
    for (const e of events) {
        out.push(...varlen(e.dt));
        switch (e.type) {
            case 'noteOn':
                out.push(0x90 | (e.ch & 0xf), e.note, e.vel ?? 100);
                break;
            case 'noteOff':
                out.push(0x80 | (e.ch & 0xf), e.note, e.vel ?? 0);
                break;
            case 'cc':
                out.push(0xb0 | (e.ch & 0xf), e.ctrl, e.val);
                break;
            case 'tempo':
                out.push(0xff, 0x51, 0x03,
                    (e.uspq >>> 16) & 0xff, (e.uspq >>> 8) & 0xff, e.uspq & 0xff);
                break;
            case 'trackName': {
                const bytes = [...e.name].map((c) => c.charCodeAt(0));
                out.push(0xff, 0x03, ...varlen(bytes.length), ...bytes);
                break;
            }
            case 'instrument': {
                const bytes = [...e.name].map((c) => c.charCodeAt(0));
                out.push(0xff, 0x04, ...varlen(bytes.length), ...bytes);
                break;
            }
            case 'rawStatus':
                out.push(e.byte);
                break;
            case 'data':
                out.push(...e.bytes);
                break;
            default:
                throw new Error(`Unknown fixture event type: ${e.type}`);
        }
    }
    // End-of-track meta event
    out.push(0, 0xff, 0x2f, 0);
    return chunk('MTrk', out);
}

export function proprietaryChunk(payload = [0, 1, 2]) {
    return chunk('XYZW', payload);
}

export function buildMidi(...parts) {
    const flat = parts.flat();
    return new Uint8Array(flat).buffer;
}
