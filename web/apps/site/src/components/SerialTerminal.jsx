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
            convertEol: false,
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
        <article>
            <header><strong>Console</strong></header>
            <div
                ref={containerRef}
                style={{
                    height: '300px',
                    width: '100%',
                    background: 'black',
                    borderRadius: '0.5rem',
                    overflow: 'hidden',
                }}
            />
        </article>
    );
}
