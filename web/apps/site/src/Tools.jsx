/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

import { MidiPlayer } from './MidiPlayer';
import { Console } from './WebConsole';
import CliReference from './content/cli.mdx';
import './Tools.scss';

export function Tools() {
    return (
        <>
            <div class="tools-page">
                <MidiPlayer collapsible />
                <Console collapsible />
            </div>
            <details>
                <summary>
                    <strong>CLI Reference</strong>
                </summary>
                <CliReference />
            </details>
        </>
    );
}
