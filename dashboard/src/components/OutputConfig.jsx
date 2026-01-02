import { GpioInput } from "./GpioInput";

export function OutputConfigSection({ channels, onChange }) {
    function updateChannel(index, value) {
        const next = [...channels];
        next[index] = value;
        onChange(next);
    }

    return (
        <article>
            <header>
                <h3>Output Channels</h3>
                <p>GPIO pins used for output channels</p>
            </header>

            <div class="channel-grid">
                {channels.map((pin, i) => (
                    <GpioInput
                        key={i}
                        id={`out-${i}`}
                        label={`Channel ${i + 1}`}
                        value={pin}
                        onChange={v => updateChannel(i, v)}
                    />
                ))}
            </div>
        </article>
    );
}
