/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

import { useState, useRef } from 'preact/hooks';
import { openEspTransport } from './services/esptool.js';
import { SerialTerminal } from './components/SerialTerminal';
import { useSerialTerminal } from './hooks/useSerialTerminal';

export function Console({ collapsible = false }) {
    const [isConnected, setConnected] = useState(false);
    const [transport, setTransport] = useState(null);
    const controllerRef = useRef(null);
    const [term, setTerm] = useState(null);

    const [collapsed, setCollapsed] = useState(
        () => collapsible && localStorage.getItem('tool:console') !== 'open'
    );

    async function connect() {
        try {
            const t = await openEspTransport();
            setTransport(t);
            setConnected(true);
        } catch (e) {
            console.error("Couldn't connect", e);
        }
    }

    async function disconnect() {
        controllerRef.current?.close();
        controllerRef.current = null;
        setTransport(null);
        setConnected(false);
    }

    async function onConnectClick() {
        if (isConnected || transport) {
            await disconnect();
        } else {
            await connect();
        }
    }

    function onToggle(e) {
        const isOpen = e.currentTarget.open;
        if (isOpen === !collapsed) return;
        setCollapsed(!isOpen);
        if (collapsible)
            localStorage.setItem('tool:console', isOpen ? 'open' : 'closed');
    }

    useSerialTerminal({ term, transport, controllerRef });

    if (!(navigator.serial || navigator.usb)) {
        return (
            <article>
                <header>
                    <h2>Console</h2>
                </header>
                <p>
                    Web Serial is not supported in this browser. Please use
                    Chrome or Edge.
                </p>
            </article>
        );
    }

    if (collapsible) {
        return (
            <article>
                <details open={!collapsed} onToggle={onToggle}>
                    <summary>Console</summary>
                    <div class="grid">
                        <button onClick={onConnectClick}>
                            {isConnected ? 'Disconnect' : 'Connect'}
                        </button>
                    </div>
                    <SerialTerminal onInit={setTerm} />
                </details>
            </article>
        );
    }

    return (
        <article>
            <header>
                <h2>Console</h2>
            </header>
            <div class="grid">
                <button onClick={onConnectClick}>
                    {isConnected ? 'Disconnect' : 'Connect'}
                </button>
            </div>
            <SerialTerminal onInit={setTerm} />
        </article>
    );
}
