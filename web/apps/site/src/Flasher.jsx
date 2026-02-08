import { useState, useRef } from 'preact/hooks';
import { firmwares } from './services/firmwares';
import { FirmwareSelector } from './components/FirmwareSelector';
import { openEspTransport, flashFirmware } from './services/esptool.js'
import { SerialTerminal } from './components/SerialTerminal';

export function Flasher() {
    const [firmwareId, setFirmwareId] = useState(firmwares[0].id);
    const [busy, setBusy] = useState(false);
    const [isConnected, setConnected] = useState(false);
    const [progress, setProgress] = useState({ phase: 'idle', percent: 0 });
    const transportRef = useRef(null);

    async function connect() {
        if (isConnected || transportRef.current) return;
        try {
            const transport = await openEspTransport();
            transportRef.current = transport;
            setConnected(true);
        } catch (e) {
            console.error("Couldn't connect", e)
        }
    }

    function log(msg) { console.log(msg); }

    async function disconnect() {
        setConnected(false);
        transportRef.current = null;
    }

    async function flash() {
        setBusy(true);
        setProgress({ phase: 'download', percent: 0 });

        const fw = firmwares.find(f => f.id === firmwareId);

        await flashFirmware({
            firmware: fw,
            transport: transportRef,
            log,
            onProgress: (phase, percent) =>
                setProgress({ phase, percent }),
        });

        log('Flash complete.');
        setBusy(false);
    }

    return (
        <article>
            <header>
                <h2>Teslasynth webtool</h2>
                <p>Flash and serial console</p>
            </header>
            <div class='grid'>
                <button onClick={connect} disabled={isConnected}>
                    Console
                </button>
                <button disabled={isConnected}>
                    Program
                </button>
            </div>

            {isConnected ?
                (<SerialTerminal
                    transport={transportRef.current}
                    onDisconnect={disconnect} />) : <div />
            }


            <FirmwareSelector firmwares={firmwares} onChange={f => setFirmwareId(f.id)} />

            <button onClick={flash} disabled={!isConnected} aria-busy={busy}>
                Flash Device
            </button>
        </article>
    );
}
