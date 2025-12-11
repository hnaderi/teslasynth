export function Slider({ label, value, min = 0, max = 100, step = 1, onChange }) {
    return (
        <label>
            {label}
            <input
                type="range"
                value={value}
                min={min}
                max={max}
                step={step}
                onInput={e => onChange(Number(e.target.value))}
            />
        </label>
    );
}

export function Switch({ label, checked, onChange }) {
    return (
        <label style="display:flex;align-items:center;gap:.5rem;">
            <input
                type="checkbox"
                role="switch"
                checked={checked}
                onInput={e => onChange(e.target.checked)}
            />
            {label}
        </label>
    );
}
