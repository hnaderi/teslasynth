export function FlashProgressBar({ progress }) {
    if (progress.phase === 'idle') return null;

    return (
        <div>
            <small>{progress.message}</small>
            <progress
                value={progress.percent}
                max="100"
            />
        </div>
    );
}
