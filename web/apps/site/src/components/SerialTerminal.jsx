/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

import { useEffect, useRef } from 'preact/hooks';
import { Terminal } from '@xterm/xterm';
import { FitAddon } from '@xterm/addon-fit';

export function SerialTerminal({ onInit }) {
    const containerRef = useRef(null);

    useEffect(() => {
        if (!containerRef.current) return;

        const terminal = new Terminal({
            cursorBlink: true,
            fontFamily: 'monospace',
            fontSize: 14,
            scrollback: 5000,
            convertEol: true,
        });

        const fit = new FitAddon();
        terminal.loadAddon(fit);
        terminal.open(containerRef.current);
        fit.fit();

        const onResize = () => fit.fit();
        window.addEventListener('resize', onResize);

        onInit(terminal);

        return () => {
            window.removeEventListener('resize', onResize);
            terminal.dispose();
        };
    }, []);

    return (
        <div
            ref={containerRef}
            style={{
                height: '300px',
                width: '100%',
                marginTop: '10px',
                background: 'black',
                borderRadius: '0.5rem',
                overflow: 'hidden',
            }}
        />
    );
}
