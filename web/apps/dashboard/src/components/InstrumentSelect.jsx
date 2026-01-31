export function InstrumentSelect({
    instruments,         // string[]
    value,               // number | null
    onChange,            // (index: number | null) => void
    id,
    label,
    disabled = false
}) {
    function handleChange(e) {
        const v = e.target.value;
        onChange(v === '' ? null : Number(v));
    }

    return (
        <label for={id}>
            {label}
            <select
                id={id}
                value={value ?? ''}
                onChange={handleChange}
                disabled={disabled}
            >
                <option value="">
                    Automatic (MIDI program)
                </option>

                {instruments.map((name, i) => (
                    <option key={i} value={i}>
                        {name}
                    </option>
                ))}
            </select>
        </label>
    );
}
