export function createSerialController(transport, { onData, onDisconnect }) {
    let closed = false;
    let writer = null;

    async function start() {
        try {
            await transport.connect(115200);
            writer = transport.device.writable ?
                transport.device.writable.getWriter() : null;

            for await (const data of transport.rawRead()) {
                if (closed || !data || data.length === 0) {
                    break;
                }
                onData?.(data);
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
        } catch (e) {
            close();
        }
    }

    function close() {
        if (closed) return;
        closed = true;

        try {
            writer?.releaseLock();
            transport.disconnect();
        } catch {}

        onDisconnect?.();
    }

    return {
        start,
        write,
        close,
    };
}
