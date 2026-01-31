export function NumberInput({ id, title, value, min, max, step, slider, onChange, help }) {
    const inputId = `${id}-input`, helperId = `${id}-helper`;
    function onInput(e) {
        if (!e.currentTarget.checkValidity()) {
            e.currentTarget.reportValidity();
            return;
        } else {
            onChange(parseFloat(e.target.value));
        }
    }
    return (
        <label>
            {title}
            {slider && <input
                type="range"
                value={value}
                min={min}
                max={max}
                step={step}
                onInput={onInput} />}
            <input
                id={inputId}
                type="number"
                value={value}
                min={min}
                max={max}
                step={step}
                required
                onInput={onInput}
                aria-describedby={helperId} />
            <small id={helperId}>
                {help}
            </small>
        </label>);
}
