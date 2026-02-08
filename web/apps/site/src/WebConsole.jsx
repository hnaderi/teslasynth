import { useState, useRef } from 'preact/hooks';
import { openEspTransport } from './services/esptool.js'
import { SerialTerminal } from './components/SerialTerminal';
import { useSerialTerminal } from './hooks/useSerialTerminal';

export function Console() {
    const [isConnected, setConnected] = useState(false);
    const [transport, setTransport] = useState(null);
    const controllerRef = useRef(null);
    const [term, setTerm] = useState(null);


    async function onClick() {
        async function connect() {
            try {
                const transport = await openEspTransport();
                setTransport(transport);
                setConnected(true);
            } catch (e) {
                console.error("Couldn't connect", e)
            }
        }

        async function disconnect() {
            controllerRef.current?.close();
            controllerRef.current = null;
            setTransport(null);
            setConnected(false);
        }

        if (isConnected || transport) {
            await disconnect();
        } else {
            await connect();
        }
    }

    useSerialTerminal({ term, transport, controllerRef });

    return (
        <article>
            <header>
                <h2>Teslasynth webtool</h2>
                <p>Flash and serial console</p>
            </header>
            <div class='grid'>
                <button onClick={onClick} >
                    {isConnected ? "Disconnect" : "Connect"}
                </button>
            </div>

            <SerialTerminal onInit={setTerm} />

        </article>
    );
}
