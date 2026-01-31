import { GpioInput } from "./GpioInput";

export function LedConfigSection({ led, onChange }) {
    return (
        <article>
            <header>
                <h3>Status LED</h3>
                <p>Optional status indicator</p>
            </header>

            <GpioInput
                id="led-pin"
                label="LED GPIO"
                value={led.pin}
                onChange={pin => onChange({ ...led, pin })}
            />

            <label>
                <input
                    type="checkbox"
                    checked={led['active-high']}
                    disabled={led.pin === -1}
                    onChange={e =>
                        onChange({
                            ...led,
                            'active-high': e.target.checked
                        })
                    }
                />
                Active high
            </label>
        </article>
    );
}
