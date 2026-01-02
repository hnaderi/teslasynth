import { useEffect, useState } from 'preact/hooks';
import { HardwareConfigForm } from './HardwareConfigForm';

export function HardwareConfigSection() {
    const [cfg, setCfg] = useState(null);
    const [busy, setBusy] = useState(false);

    useEffect(() => {
        fetch('/api/config/hardware')
            .then(r => r.json())
            .then(setCfg);
    }, []);

    if (!cfg) {
        return (
            <article>
                <header aria-busy="true">
                    Loading Hardware Configuration…
                </header>
            </article>
        );
    }

    return (
        <article>
            <header>
                <hgroup>
                    <h2>Hardware Configuration</h2>
                    <p>GPIO and hardware routing</p>
                </hgroup>
            </header>

            <HardwareConfigForm
                config={cfg}
                busy={busy}
                setBusy={setBusy}
                onUpdate={setCfg}
                onReset={setCfg}
            />
        </article>
    );
}
