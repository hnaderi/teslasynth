/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

export function createSerialController(transport, { onData, onDisconnect }) {
    let closed = false;
    let writer = null;

    async function start() {
        try {
            await transport.connect(115200);
            writer = transport.device.writable
                ? transport.device.writable.getWriter()
                : null;

            for await (const data of transport.rawRead()) {
                if (closed) break;
                // Empty chunks can appear during idle, USB buffer flushes, or
                // brief driver hiccups — they are NOT a disconnect signal.
                if (data && data.length > 0) onData?.(data);
            }
        } catch (e) {
            if (!closed) {
                console.warn('Serial read error', e);
            }
        } finally {
            close();
        }
    }

    async function write(data) {
        if (closed) return;
        try {
            if (writer) {
                await writer.write(data);
            }
        } catch {
            close();
        }
    }

    function close() {
        if (closed) return;
        closed = true;

        try {
            writer?.releaseLock();
            transport.disconnect();
        } catch {
            /* ignore cleanup errors */
        }

        onDisconnect?.();
    }

    return {
        start,
        write,
        close,
    };
}
