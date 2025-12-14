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
                        type="number"
                        value={cfg.tuning}
                        onInput={e => setCfg({ ...cfg, tuning: parseFloat(e.target.value) })}
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
                                    type="number"
                                    value={ch['max-on-time']}
                                    onInput={e => updateChannel(cfg, setCfg, idx, 'max-on-time', parseInt(e.target.value))}
                                />
                            </label>

                            <label>
                                Min Dead-Time
                                <input
                                    type="number"
                                    value={ch['min-deadtime']}
                                    onInput={e => updateChannel(cfg, setCfg, idx, 'min-deadtime', parseInt(e.target.value))}
                                />
                            </label>

                            <label>
                                Max Duty
                                <input
                                    type="number"
                                    value={ch['max-duty']}
                                    onInput={e => updateChannel(cfg, setCfg, idx, 'max-duty', parseFloat(e.target.value))}
                                />
                            </label>

                            <label>
                                Duty Window
                                <input
                                    type="number"
                                    value={ch['duty-window']}
                                    onInput={e => updateChannel(cfg, setCfg, idx, 'duty-window', parseInt(e.target.value))}
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
