/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

import { describe, it, expect } from 'vitest';
import { createSerialController } from './serialController';

function makeMockTransport(chunks) {
    let connected = false;
    let disconnected = false;
    return {
        device: { writable: null }, // exercise the writer-null branch
        connect: async () => {
            connected = true;
        },
        disconnect: () => {
            disconnected = true;
        },
        async *rawRead() {
            for (const c of chunks) yield c;
        },
        get connected() {
            return connected;
        },
        get disconnected() {
            return disconnected;
        },
    };
}

describe('createSerialController', () => {
    it('does NOT disconnect when the stream emits empty chunks', async () => {
        // Regression: the previous implementation broke out of the read loop on
        // any zero-length chunk, causing intermittent random disconnects.
        const transport = makeMockTransport([
            new Uint8Array([1, 2, 3]),
            new Uint8Array(0), // empty — must not terminate the loop
            new Uint8Array([4, 5]),
        ]);
        const received = [];
        let disconnected = false;
        const ctl = createSerialController(transport, {
            onData: (d) => received.push(...d),
            onDisconnect: () => {
                disconnected = true;
            },
        });

        await ctl.start();

        // All non-empty chunks made it through.
        expect(received).toEqual([1, 2, 3, 4, 5]);
        // The empty chunk was skipped, not delivered.
        expect(received).not.toContain(undefined);
        // Disconnect happens only at end-of-stream (controller is single-use).
        expect(disconnected).toBe(true);
    });

    it('skips falsy chunks without invoking onData', async () => {
        const transport = makeMockTransport([
            null,
            undefined,
            new Uint8Array([42]),
        ]);
        const received = [];
        const ctl = createSerialController(transport, {
            onData: (d) => received.push(...d),
            onDisconnect: () => {},
        });
        await ctl.start();
        expect(received).toEqual([42]);
    });

    it('close() is idempotent and onDisconnect fires exactly once', async () => {
        const transport = makeMockTransport([new Uint8Array([1])]);
        let disconnectCount = 0;
        const ctl = createSerialController(transport, {
            onData: () => {},
            onDisconnect: () => {
                disconnectCount++;
            },
        });
        await ctl.start();
        ctl.close();
        ctl.close();
        expect(disconnectCount).toBe(1);
    });

    it('write() is a no-op after close', async () => {
        const transport = makeMockTransport([]);
        const ctl = createSerialController(transport, {
            onData: () => {},
            onDisconnect: () => {},
        });
        await ctl.start();
        // Stream exhausted → controller closed.
        await expect(ctl.write(new Uint8Array([1]))).resolves.toBeUndefined();
    });
});
