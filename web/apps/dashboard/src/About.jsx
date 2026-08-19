/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

import { Logo } from '@teslasynth/ui/components/Logo';
import { Modal } from './components/modal';

export function About({ open, onClose }) {
    return (
        <Modal title="About" open={open} onClose={onClose}>
            <p>
                {' '}
                <Logo size={100} />
            </p>
            <p>
                MIDI synthesizer that plays music through any interrupted
                device, including Tesla coils, Flyback transformers, High power
                lasers, ...
            </p>
            <ul>
                <li>
                    Author: <a href="https://hnaderi.dev">Hossein Naderi</a>
                </li>
                <li>
                    Source code:{' '}
                    <a href="https://github.com/hnaderi/teslasynth">github</a>
                </li>
                <li>
                    Contact:{' '}
                    <a href="mailto:mail@hnaderi.dev">mail@hnaderi.dev</a>
                </li>
            </ul>
        </Modal>
    );
}
