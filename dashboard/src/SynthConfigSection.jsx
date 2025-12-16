import { useEffect, useState } from 'preact/hooks';
const synthConfig = (init) => fetch('/api/config/synth', init)

function NumberInput({ id, title, value, min, max, step, slider, onChange, help }) {
    const inputId = `${id}-input`, helperId = `${id}-helper`;
    function onInput(e) {
        if (!e.currentTarget.checkValidity()) {
            e.currentTarget.reportValidity();
            return;
        } else {
            onChange(parseFloat(e.target.value))
        }
    }
    return (
        <label>
            {title}
            {
                slider && <input
                    type="range"
                    value={value}
                    min={min}
                    max={max}
                    step={step}
                    onInput={onInput}
                />
            }
            <input
                id={inputId}
                type="number"
                value={value}
                min={min}
                max={max}
                step={step}
                required
                onInput={onInput}
                aria-describedby={helperId}
            />
            <small id={helperId}>
                {help}
            </small>
        </label>)
}
function SynthChannelConfigSection({ channel, channelIdx, onChange }) {
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

export function SynthConfigSection({ requestConfirm }) {
    const [cfg, setCfg] = useState(null);

    useEffect(() => {
        synthConfig()
            .then(r => r.json())
            .then(setCfg);
    }, []);

    if (!cfg) return <article><header aria-busy="true">Loading Synth Configuration…</header></article>;

    function onSubmit(e) {
        e.preventDefault();

        const form = e.currentTarget.form;
        if (!form.checkValidity()) {
            form.reportValidity();
            return;
        }
        saveSynth(cfg)
    }

    function onReset(e) {
        e.preventDefault();

        requestConfirm({
            title: "Reset to factory settings",
            message: "This will delete all your settings permanently, are you sure?",
            onConfirm: resetSynthConfig
        })
    }

    return (
        <article>
            <header>
                <hgroup>
                    <h2>Synth Configuration</h2>
                    <p>Base tuning and per-channel timing parameters</p>
                </hgroup>
            </header>

            <form>
                <NumberInput
                    id="tuning"
                    title="Tuning Frequency (A4)"
                    help="A4 note frequency in hertz"
                    value={cfg.tuning}
                    min="1"
                    max="1000"
                    step="0.001"
                    onChange={n => setCfg({ ...cfg, tuning: n })}
                />

                <h3>Channels</h3>

                <div class="channel-grid">
                    {cfg.channels.map((ch, idx) => (
                        SynthChannelConfigSection({
                            channel: ch, channelIdx: idx,
                            onChange: (idx, field, value) =>
                                updateChannel(cfg, setCfg, idx, field, value),
                        })
                    ))}
                </div>

                <footer>
                    <div class="grid">
                        <button type="submit" onClick={onSubmit}>Save</button>
                        <button type="button" onClick={onReset}>Reset</button>
                    </div>
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
    synthConfig({
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(cfg)
    });
}

function resetSynthConfig() {
    synthConfig({
        method: 'DELETE',
        headers: { 'Content-Type': 'application/json' },
    });
}
