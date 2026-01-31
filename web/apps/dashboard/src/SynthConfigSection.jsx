import { useEffect, useState } from 'preact/hooks';
import { NumberInput } from './components/NumberInput';
import { ConfirmDialog } from './components/confirmation';
import { InstrumentSelect } from './components/InstrumentSelect';
import { RoutingConfigSection } from './components/RoutingConfigSection';
const synthConfig = (init) => fetch('/api/config/synth', init)

function SynthChannelConfigSection({ channel, channelIdx, onChange, instruments }) {
    return (
        <article class="channel" key={channelIdx} style="padding: 1rem;">
            <header><strong>Channel {channelIdx + 1}</strong></header>

            <NumberInput
                id={`notes-${channelIdx}`}
                title="Max concurrent notes"
                help="Maximum allowed number of notes playing at the same time"
                value={channel.notes}
                onChange={n => onChange(channelIdx, 'notes', n)}
                min="1"
                max="4"
                step="1"
            />

            <InstrumentSelect
                id={`instrument-${channelIdx}`}
                instruments={instruments}
                label="Instrument"
                onChange={id => onChange(channelIdx, 'instrument', id)}
                value={channel['instrument']}
            />

            <NumberInput
                id={`max-on-time-${channelIdx}`}
                title="Max On-Time (us)"
                help="Maximum allowed on-time for each pulse in microseconds"
                value={channel['max-on-time']}
                min="0"
                max="65536"
                step="1"
                onChange={n => onChange(channelIdx, 'max-on-time', n)}
            />

            <NumberInput
                id={`min-deadtime-${channelIdx}`}
                title="Min Dead-Time (us)"
                help="Minimum deadtime that must be guaranteed in microseconds"
                value={channel['min-deadtime']}
                min="0"
                max="65536"
                step="1"
                onChange={n => onChange(channelIdx, 'min-deadtime', n)}
            />

            <NumberInput
                id={`max-dutycycle-${channelIdx}`}
                title="Max Duty (%)"
                help="Maximum allowed duty cycle for this channel in percent"
                value={channel['max-duty']}
                min="0"
                max="100"
                step="0.5"
                slider={true}
                onChange={n => onChange(channelIdx, 'max-duty', n)}
            />

            <NumberInput
                id={`duty-window-${channelIdx}`}
                title="Duty Window (us)"
                help="Time frame that the duty cycle limitation is enforced in microseconds."
                value={channel['duty-window']}
                min="10000"
                max="65536"
                step="1"
                onChange={n => onChange(channelIdx, 'duty-window', n)}
            />
        </article>
    )
}

function SynthConfigForm({
    config,
    busy,
    onUpdate,
    onReset,
    setBusy,
    instruments,
}) {
    const [draft, setDraft] = useState(config);
    const [confirmOpen, setConfirmOpen] = useState(false);

    useEffect(() => {
        setDraft(config);
    }, [config]);

    async function save(e) {
        e.preventDefault();

        const form = e.currentTarget.form;
        if (!form.checkValidity()) {
            form.reportValidity();
            return;
        }
        setBusy(true);
        try {
            const res = await synthConfig({
                method: 'PUT',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(draft)
            });
            const updated = await res.json();
            onUpdate(updated);
        } finally {
            setBusy(false);
        }
    }

    async function reset() {
        setBusy(true)
        try {
            const res = await synthConfig({ method: 'DELETE' })
            const updated = await res.json()
            onReset(updated)
        } finally {
            setBusy(false)
            setConfirmOpen(false)
        }
    }

    return (
        <form>
            <NumberInput
                id="tuning"
                title="Tuning Frequency (A4)"
                help="A4 note frequency in hertz"
                value={draft.tuning}
                min="1"
                max="1000"
                step="0.001"
                onChange={n => setDraft({ ...draft, tuning: n })}
            />
            <InstrumentSelect
                id="instrument"
                instruments={instruments}
                label="Global instrument"
                onChange={id => setDraft({ ...draft, instrument: id })}
                value={draft['instrument']}
            />

            <h3>Channels</h3>

            <div class="channel-grid">
                {draft.channels.map((ch, idx) => (
                    SynthChannelConfigSection({
                        channel: ch, channelIdx: idx,
                        onChange: (idx, field, value) =>
                            updateChannel(draft, setDraft, idx, field, value),
                        instruments: instruments
                    })
                ))}
            </div>

            <RoutingConfigSection
                routing={draft.routing}
                channelCount={draft.channels.length}
                onChange={routing =>
                    setDraft({ ...draft, routing })
                }
            />

            <footer>
                <div class="grid">
                    <button type="submit" onClick={save} disabled={busy}>Save</button>
                    <button type="button" onClick={() => setConfirmOpen(true)} disabled={busy}>Reset</button>
                    <ConfirmDialog
                        open={confirmOpen}
                        title="Reset configuration?"
                        message="This will erase all settings permanently."
                        busy={busy}
                        onCancel={() => setConfirmOpen(false)}
                        onConfirm={reset}
                    />
                </div>
            </footer>
        </form>
    );
}


export function SynthConfigSection() {
    const [cfg, setCfg] = useState(null);
    const [instruments, setInstruments] = useState(null);
    const [busy, setBusy] = useState(false);

    useEffect(() => {
        synthConfig()
            .then(r => r.json())
            .then(setCfg);

        fetch('/api/synth/instruments')
            .then(r => r.json())
            .then(setInstruments);
    }, []);

    if (!cfg || !instruments)
        return <article><header aria-busy="true">Loading Synth Configuration…</header></article>;
    else
        return (
            <article>
                <header>
                    <hgroup>
                        <h2>Synth Configuration</h2>
                        <p>Base tuning and per-channel timing parameters</p>
                    </hgroup>
                </header>
                <SynthConfigForm
                    config={cfg}
                    onReset={setCfg}
                    onUpdate={setCfg}
                    busy={busy}
                    setBusy={setBusy}
                    instruments={instruments}
                />
            </article>
        );
}

function updateChannel(cfg, setCfg, index, field, value) {
    const updated = [...cfg.channels];
    updated[index] = { ...updated[index], [field]: value };
    setCfg({ ...cfg, channels: updated });
}
