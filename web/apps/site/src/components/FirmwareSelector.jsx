export function FirmwareSelector({ firmwares, value, onChange }) {
    return (
        <label>
            Firmware
            <select value={value} onChange={e => onChange(e.target.value)}>
                {firmwares.map(f => (
                    <option value={f.id}>
                        {f.name} ({f.version})
                    </option>
                ))}
            </select>
        </label>
    );
}
