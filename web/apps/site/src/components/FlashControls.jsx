export function FlashControls({ onFlash, busy }) {
    return (
        <button
            type="button"
            aria-busy={busy}
            onClick={onFlash}
        >
            Flash Device
        </button>
    );
}
