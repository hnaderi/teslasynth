export function BooleanIndicator({ value, label }) {
    return (
        <span
            role="status"
            aria-label={`${label}: ${value ? 'available' : 'not available'}`}
            class={value ? 'bool-yes' : 'bool-no'}
        >
            {value ? '✓' : '✕'}
        </span>
    );
}
