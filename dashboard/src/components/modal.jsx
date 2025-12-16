import { useEffect, useRef } from 'preact/hooks';

export function Modal({ open, onClose, title, children }) {
    const dialogRef = useRef(null);

    useEffect(() => {
        const dialog = dialogRef.current;
        if (!dialog) return;

        if (open && !dialog.open) {
            dialog.showModal();
        } else if (!open && dialog.open) {
            dialog.close();
        }
    }, [open]);

    function handleCancel(e) {
        e.preventDefault();
        onClose?.();
    }

    function handleClose() {
        onClose?.();
    }

    return (
        <dialog ref={dialogRef} onCancel={handleCancel} onClose={handleClose}>
            <article>
                <header>
                    <button
                        aria-label="Close"
                        rel="prev"
                        onClick={() => dialogRef.current.close()}
                    ></button>
                    <p>
                        <strong>{title}</strong>
                    </p>
                </header>
                {children}
            </article>
        </dialog>
    );
}
