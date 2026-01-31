export function GpioInput({
    id,
    label,
    value,
    onChange,
    allowNone = true,
    min = 0,
    max = 39
}) {
    function handleChange(e) {
        const v = e.target.value;
        onChange(v === '' ? -1 : Number(v));
    }

    return (
        <label for={id}>
            {label}
            <input
                id={id}
                type="number"
                min={allowNone ? undefined : min}
                max={max}
                step="1"
                value={value === -1 ? '' : value}
                placeholder={allowNone ? 'Not connected' : undefined}
                onChange={handleChange}
            />
        </label>
    );
}
