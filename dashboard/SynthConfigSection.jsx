import { useEffect, useState } from 'preact/hooks';

export function SynthConfigSection() {
    const [cfg, setCfg] = useState(null);

    useEffect(() => {
        fetch('/api/config/synth')
            .then(r => r.json())
            .then(setCfg);
    }, []);

    if (!cfg) return <article><header>Loading Synth Configuration…</header></article>;

    return (
        <article>
            <header>
                <hgroup>
                    <h2>Synth Configuration</h2>
                    <p>Base tuning and per-channel timing parameters</p>
                </hgroup>
            </header>

            <form>
                <label>
                    Tuning Frequency (A4)
                    <input
                        name="tuning"
                        type="text"
                        value={cfg.tuning}
                        onInput={e => setCfg({ ...cfg, tuning: e.target.value })}
                    />
                </label>

                <h3>Channels</h3>

                <div class="grid">
                    {cfg.channels.map((ch, idx) => (
                        <article class="channel" key={idx} style="padding: 1rem;">
                            <header><strong>Channel {idx + 1}</strong></header>

                            <label>
                                Notes
                                <input
                                    type="number"
                                    value={ch.notes}
                                    onInput={e => updateChannel(cfg, setCfg, idx, 'notes', parseInt(e.target.value))}
                                />
                            </label>

                            <label>
                                Max On-Time
                                <input
                                    type="text"
                                    value={ch['max-on-time']}
                                    onInput={e => updateChannel(cfg, setCfg, idx, 'max-on-time', e.target.value)}
                                />
                            </label>

                            <label>
                                Min Dead-Time
                                <input
                                    type="text"
                                    value={ch['min-dead-time']}
                                    onInput={e => updateChannel(cfg, setCfg, idx, 'min-dead-time', e.target.value)}
                                />
                            </label>

                            <label>
                                Max Duty
                                <input
                                    type="text"
                                    value={ch['max-duty']}
                                    onInput={e => updateChannel(cfg, setCfg, idx, 'max-duty', e.target.value)}
                                />
                            </label>

                            <label>
                                Duty Window
                                <input
                                    type="text"
                                    value={ch['duty-window']}
                                    onInput={e => updateChannel(cfg, setCfg, idx, 'duty-window', e.target.value)}
                                />
                            </label>
                        </article>
                    ))}
                </div>

                <footer>
                    <button type="button" onClick={() => saveSynth(cfg)}>Save Configuration</button>
                </footer>
            </form>
        </article>
    );
}

function updateChannel(cfg, setCfg, index, field, value) {
    const updated = [...cfg.channels];
    updated[index] = { ...updated[index], [field]: value };
    setCfg({ ...cfg, channels: updated });
}

function saveSynth(cfg) {
    fetch('/api/config/synth', {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(cfg)
    });
}
