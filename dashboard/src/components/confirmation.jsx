import { useEffect, useRef } from 'preact/hooks';

export function ConfirmDialog({
    open,
    title = 'Confirm',
    message,
    confirmText = 'Confirm',
    cancelText = 'Cancel',
    onConfirm,
    onCancel
}) {
    const dialogRef = useRef(null);

    useEffect(() => {
        const dialog = dialogRef.current;
        if (!dialog) return;

        if (open && !dialog.open) dialog.showModal();
        if (!open && dialog.open) dialog.close();
    }, [open]);

    function close() {
        dialogRef.current?.close();
    }

    return (
        <dialog ref={dialogRef} onClose={onCancel}>
            <article>
                <header>
                    <h3>{title}</h3>
                </header>

                <p>{message}</p>

                <footer>
                    <button
                        class="secondary"
                        onClick={() => {
                            close();
                            onCancel?.();
                        }}
                    >
                        {cancelText}
                    </button>

                    <button
                        onClick={() => {
                            close();
                            onConfirm?.();
                        }}
                    >
                        {confirmText}
                    </button>
                </footer>
            </article>
        </dialog>
    );
}
