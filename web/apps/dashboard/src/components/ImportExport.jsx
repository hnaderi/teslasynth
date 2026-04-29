/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Shared Import/Export controls for config sections. Exports trigger a
 * browser download; imports parse a local JSON file and hand the decoded
 * object back to the caller (typically wired to a setDraft setter, so the
 * user still has to click Save to push it to the device).
 */

import { useRef } from 'preact/hooks';

export function exportJson(filename, data) {
    const blob = new Blob([JSON.stringify(data, null, 2) + '\n'], {
        type: 'application/json',
    });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
}

export function ImportExportButtons({
    filename,
    data,
    onImport,
    busy,
    importLabel = 'Import',
    exportLabel = 'Export',
}) {
    const fileInputRef = useRef(null);

    function handleFile(e) {
        const file = e.target.files?.[0];
        if (!file) return;
        const reader = new FileReader();
        reader.onload = () => {
            try {
                const parsed = JSON.parse(reader.result);
                onImport(parsed);
            } catch (err) {
                alert(`Invalid JSON: ${err.message}`);
            }
        };
        reader.readAsText(file);
        e.target.value = '';
    }

    return (
        <>
            <button
                type="button"
                class="secondary"
                onClick={() => exportJson(filename, data)}
                disabled={busy}
            >
                {exportLabel}
            </button>
            <button
                type="button"
                class="secondary"
                onClick={() => fileInputRef.current?.click()}
                disabled={busy}
            >
                {importLabel}
            </button>
            <input
                ref={fileInputRef}
                type="file"
                accept="application/json,.json"
                style="display: none"
                onChange={handleFile}
            />
        </>
    );
}
