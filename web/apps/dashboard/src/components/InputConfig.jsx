/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

import { GpioInput } from './GpioInput';

export function InputConfigSection({ input, onChange }) {
    return (
        <article>
            <header>
                <h3>Input</h3>
                <p>Primary input pin</p>
            </header>

            <GpioInput
                id="input-pin"
                label="Input GPIO"
                value={input.pin}
                onChange={(pin) => onChange({ ...input, pin })}
            />
        </article>
    );
}
