import { useEffect, useState } from "preact/hooks";
import { DisplayConfigSection } from "./DisplayConfig";
import { InputConfigSection } from "./InputConfig";
import { LedConfigSection } from "./LEDConfig";
import { OutputConfigSection } from "./OutputConfig";

export function HardwareConfigForm({
    config,
    busy,
    setBusy,
    onUpdate,
    onReset
}) {
    const [draft, setDraft] = useState(config);
    const [confirmOpen, setConfirmOpen] = useState(false);

    useEffect(() => setDraft(config), [config]);

    async function save(e) {
        e.preventDefault();
        setBusy(true);
        try {
            const res = await fetch('/api/config/hardware', {
                method: 'PUT',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(draft)
            });
            onUpdate(await res.json());
        } finally {
            setBusy(false);
        }
    }

    async function reset() {
        setBusy(true);
        try {
            const res = await fetch('/api/config/hardware', {
                method: 'DELETE'
            });
            onReset(await res.json());
        } finally {
            setBusy(false);
            setConfirmOpen(false);
        }
    }

    return (
        <form onSubmit={save}>
            <DisplayConfigSection display={draft.display} />

            <OutputConfigSection
                channels={draft.output.channels}
                onChange={channels =>
                    setDraft({
                        ...draft,
                        output: { ...draft.output, channels }
                    })
                }
            />

            <InputConfigSection
                input={draft.input}
                onChange={input =>
                    setDraft({ ...draft, input })
                }
            />

            <LedConfigSection
                led={draft.led}
                onChange={led =>
                    setDraft({ ...draft, led })
                }
            />

            <footer>
                <div class="grid">
                    <button type="submit" disabled={busy}>
                        Save
                    </button>
                    <button
                        type="button"
                        disabled={busy}
                        onClick={() => setConfirmOpen(true)}
                    >
                        Reset
                    </button>
                </div>
            </footer>
        </form>
    );
}
