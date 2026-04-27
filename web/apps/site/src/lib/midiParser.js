/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

/**
 * Parse an SMF (Standard MIDI File) from an ArrayBuffer.
 * Returns a flat sorted event list with absolute µs timestamps.
 */
export function parseMidi(buffer) {
    const data = new DataView(buffer);
    let pos = 0;

    const u32 = () => {
        const v = data.getUint32(pos);
        pos += 4;
        return v;
    };
    const u16 = () => {
        const v = data.getUint16(pos);
        pos += 2;
        return v;
    };
    const u8 = () => data.getUint8(pos++);
    const varlen = () => {
        let v = 0,
            b;
        do {
            b = u8();
            v = (v << 7) | (b & 0x7f);
        } while (b & 0x80);
        return v;
    };
    const skip = (n) => {
        pos += n;
    };

    if (u32() !== 0x4d546864) throw new Error('Not a MIDI file');
    const hdrLen = u32();
    const format = u16();
    const nTracks = u16();
    const timeDivision = u16();
    if (timeDivision & 0x8000)
        throw new Error('SMPTE time division not supported');
    const ticksPerQN = timeDivision;
    pos = 8 + hdrLen; // skip any extended header bytes (normally 0)

    const tracks = [];

    // Iterate all chunks; skip unknown ones (some DAWs insert proprietary chunks)
    while (pos + 8 <= data.byteLength && tracks.length < nTracks) {
        const chunkType = u32();
        const chunkLen = u32();
        const trackEnd = pos + chunkLen;

        if (chunkType !== 0x4d54726b) {
            pos = trackEnd;
            continue;
        } // skip non-MTrk

        const events = [];
        let tick = 0,
            running = 0;

        while (pos < trackEnd) {
            tick += varlen();
            let status = data.getUint8(pos);

            if (status & 0x80) {
                running = status;
                pos++;
            } else {
                status = running;
            }

            const msgType = status & 0xf0;
            const ch = status & 0x0f;

            if (status === 0xff) {
                // Always read all bytes first, then inspect — same as original PoC
                const meta = u8();
                const bytes = new Array(varlen()).fill(0).map(() => u8());
                if (meta === 0x51 && bytes.length === 3) {
                    const uspq = (bytes[0] << 16) | (bytes[1] << 8) | bytes[2];
                    events.push({ tick, type: 'tempo', uspq });
                } else if (meta === 0x03 || meta === 0x04) {
                    // 0x03 = Sequence/Track Name, 0x04 = Instrument Name
                    const name = bytes
                        .filter((b) => b > 0)
                        .map((b) => String.fromCharCode(b))
                        .join('')
                        .trim();
                    if (name) events.push({ tick, type: 'trackName', name });
                }
            } else if (status === 0xf0 || status === 0xf7) {
                skip(varlen());
            } else if (msgType >= 0x80 && msgType <= 0xe0) {
                const b1 = u8();
                const b2 = msgType !== 0xc0 && msgType !== 0xd0 ? u8() : 0;
                events.push({
                    tick,
                    type: 'midi',
                    status,
                    ch,
                    b1,
                    b2,
                    msgType,
                });
            }
            // 0xf1–0xfe (system realtime / common) have no data bytes — skip silently
        }
        pos = trackEnd;
        tracks.push(events);
    } // end chunk loop

    // Merge all tracks and sort; tempo events win ties so µs math stays correct
    const merged = tracks.flat();
    merged.sort((a, b) => a.tick - b.tick || (a.type === 'tempo' ? -1 : 1));

    // Resolve ticks → µs
    let curTick = 0,
        curUs = 0,
        curUspq = 500000;
    const events = merged.map((ev) => {
        curUs += (ev.tick - curTick) * (curUspq / ticksPerQN);
        curTick = ev.tick;
        if (ev.type === 'tempo') curUspq = ev.uspq;
        return { ...ev, us: curUs };
    });

    const endUs = events.reduce((m, e) => (e.us > m ? e.us : m), 0);

    const trackNames = tracks.map((t, i) => {
        const nm = t.find((e) => e.type === 'trackName');
        return nm?.name || `Track ${i + 1}`;
    });
    const trackChannels = tracks.map((t) => [
        ...new Set(t.filter((e) => e.type === 'midi').map((e) => e.ch)),
    ]);

    const tempoEvents = events.filter((e) => e.type === 'tempo');
    const initBPM = Math.round(60000000 / (tempoEvents[0]?.uspq ?? 500000));

    return {
        format,
        nTracks,
        ticksPerQN,
        trackNames,
        trackChannels,
        initBPM,
        endUs,
        events,
        tempoEvents,
    };
}
