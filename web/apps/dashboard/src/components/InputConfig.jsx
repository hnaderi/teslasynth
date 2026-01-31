import { GpioInput } from "./GpioInput";

export function InputConfigSection({ input, onChange }) {
    return (
        <article>
            <header>
                <h3>Input</h3>
                <p>Primary input pin</p>
            </header>

            <GpioInput
                id="input-pin"
                label="Input GPIO"
                value={input.pin}
                onChange={pin => onChange({ ...input, pin })}
            />
        </article>
    );
}
