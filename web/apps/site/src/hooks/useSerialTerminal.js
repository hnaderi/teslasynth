import { useEffect } from 'preact/hooks';
import { createSerialController } from '../services/serialController';

export function useSerialTerminal({ term, transport, controllerRef }) {
    useEffect(() => {
        if (!term || !transport) return;

        const controller = createSerialController(transport, {
            onData: data => term.write(data),
            onDisconnect: () => {
                term.writeln('\r\n[Disconnected]');
            }
        });

        controllerRef.current = controller;

        const encoder = new TextEncoder();
        const disposable = term.onData(data => {
            controller.write(encoder.encode(data));
        });

        controller.start();

        return () => {
            disposable.dispose();
            controller.close();
            controllerRef.current = null;
        };
    }, [term, transport]);
}
