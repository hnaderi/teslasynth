/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

import { useEffect, useState } from 'preact/hooks';
import { InputConfigSection } from './InputConfig';
import { LedConfigSection } from './LEDConfig';
import { OutputConfigSection } from './OutputConfig';
import { ImportExportButtons } from './ImportExport';
import { ConfigForm, ConfigFormActions, useConfigEndpoint } from './ConfigForm';

export function HardwareConfigForm({ config, busy, setBusy, onUpdate }) {
    const [draft, setDraft] = useState(config);
    const { error, save, reset } = useConfigEndpoint(
        '/api/config/hardware',
        setBusy,
        onUpdate
    );

    useEffect(() => setDraft(config), [config]);

    return (
        <ConfigForm>
            <OutputConfigSection
                channels={draft.output.channels}
                onChange={(channels) =>
                    setDraft({
                        ...draft,
                        output: { ...draft.output, channels },
                    })
                }
            />

            <InputConfigSection
                input={draft.input}
                onChange={(input) => setDraft({ ...draft, input })}
            />

            <LedConfigSection
                led={draft.led}
                onChange={(led) => setDraft({ ...draft, led })}
            />

            <ConfigFormActions
                busy={busy}
                error={error}
                onSave={() => save(draft)}
                onReset={reset}
                resetTitle="Reset hardware configuration?"
                resetMessage="This restores the factory GPIO assignments. Continue?"
            >
                <ImportExportButtons
                    filename="hardware-config.json"
                    data={draft}
                    onImport={setDraft}
                    busy={busy}
                />
            </ConfigFormActions>
        </ConfigForm>
    );
}
