export function ConfirmDialog({
    open,
    title,
    message,
    onConfirm,
    onCancel,
    busy = false
}) {
    if (!open) return null;

    return (
        <dialog open>
            <article>
                <header>
                    <h3>{title}</h3>
                </header>

                <p>{message}</p>

                <footer>
                    <button
                        class="secondary"
                        onClick={onCancel}
                        disabled={busy}
                    >
                        Cancel
                    </button>

                    <button
                        class="contrast"
                        onClick={onConfirm}
                        disabled={busy}
                    >
                        Confirm
                    </button>
                </footer>
            </article>
        </dialog>
    );
}
