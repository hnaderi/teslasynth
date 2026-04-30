/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

import { useEffect, useState } from 'preact/hooks';
import { Logo } from '@teslasynth/ui/components/Logo';
import { Router, route } from 'preact-router';
import { MDXProvider } from '@mdx-js/preact';
import { Home } from './Home';
import GettingStarted from './content/getting_started.mdx';
import Boards from './content/boards.mdx';
import Midi from './content/midi.mdx';
import Configuration from './content/configuration.mdx';
import Python from './content/python.mdx';
import Building from './content/building.mdx';
import { Console } from './WebConsole';
import { FirmwareFlasher } from './Flasher';
import { MidiPlayer } from './MidiPlayer';
import { Tools } from './Tools';
import { PwaInstallButton } from './components/PwaInstallButton';

export default function App() {
    const [mobileOpen, setMobileOpen] = useState(false);

    const BASE_URL = import.meta.env.BASE_URL;
    const home_url = BASE_URL;
    const getting_started_url = BASE_URL + 'getting-started';
    const boards_url = BASE_URL + 'boards';
    const midi_url = BASE_URL + 'midi';
    const configuration_url = BASE_URL + 'configuration';
    const python_url = BASE_URL + 'python';
    const building_url = BASE_URL + 'building';
    const console_url = BASE_URL + 'console';
    const flasher_url = BASE_URL + 'flasher';
    const midi_player_url = BASE_URL + 'midi-player';
    const tools_url = BASE_URL + 'tools';

    useEffect(() => {
        const params = new URLSearchParams(window.location.search);
        const redirected = params.get('p');

        if (redirected) {
            history.replaceState(null, '', import.meta.env.BASE_URL);
            route(redirected, true);
        }
    }, []);

    return (
        <MDXProvider
            components={{
                wrapper: ({ children }) => <article>{children}</article>,
                a: ({ href, ...props }) => (
                    <a
                        href={
                            href?.startsWith('/')
                                ? BASE_URL.replace(/\/$/, '') + href
                                : href
                        }
                        {...props}
                    />
                ),
            }}
        >
            <nav>
                <ul>
                    <Logo size={100} />
                </ul>
                <ul class="nav-links">
                    <li>
                        <a href={home_url}>Home</a>
                    </li>
                    <li>
                        <details class="dropdown">
                            <summary>Docs</summary>
                            <ul dir="rtl">
                                <li dir="ltr">
                                    <a href={getting_started_url}>
                                        Getting started
                                    </a>
                                </li>
                                <li dir="ltr">
                                    <a href={boards_url}>Choosing a board</a>
                                </li>
                                <li dir="ltr">
                                    <a href={midi_url}>MIDI setup</a>
                                </li>
                                <li dir="ltr">
                                    <a href={configuration_url}>
                                        Configuration
                                    </a>
                                </li>
                                <li dir="ltr">
                                    <a href={python_url}>Python</a>
                                </li>
                                <li dir="ltr">
                                    <a href={building_url}>Building</a>
                                </li>
                            </ul>
                        </details>
                    </li>
                    <li>
                        <a href={tools_url}>Tools</a>
                    </li>
                    <li>
                        <a
                            href="https://github.com/hnaderi/teslasynth"
                            target="_blank"
                        >
                            GitHub
                        </a>
                    </li>
                    <li>
                        <PwaInstallButton />
                    </li>
                </ul>
                <div class="nav-mobile">
                    <button
                        class="outline secondary nav-hamburger"
                        onClick={() => setMobileOpen((o) => !o)}
                        aria-label="Menu"
                        aria-expanded={mobileOpen}
                    >
                        {mobileOpen ? '✕' : '☰'}
                    </button>
                </div>
            </nav>

            {mobileOpen && (
                <nav class="nav-mobile-expanded">
                    <ul onClick={() => setMobileOpen(false)}>
                        <li>
                            <a href={home_url}>Home</a>
                        </li>
                        <li>
                            <a href={getting_started_url}>Getting started</a>
                        </li>
                        <li>
                            <a href={boards_url}>Choosing a board</a>
                        </li>
                        <li>
                            <a href={midi_url}>MIDI setup</a>
                        </li>
                        <li>
                            <a href={configuration_url}>Configuration</a>
                        </li>
                        <li>
                            <a href={python_url}>Python</a>
                        </li>
                        <li>
                            <a href={building_url}>Building</a>
                        </li>
                        <li>
                            <a href={tools_url}>Tools</a>
                        </li>
                        <li>
                            <a
                                href="https://github.com/hnaderi/teslasynth"
                                target="_blank"
                            >
                                GitHub
                            </a>
                        </li>
                        <li>
                            <PwaInstallButton />
                        </li>
                    </ul>
                </nav>
            )}

            <main class="container">
                <Router>
                    <Home path={home_url} />
                    <GettingStarted path={getting_started_url} />
                    <Boards path={boards_url} />
                    <Midi path={midi_url} />
                    <Configuration path={configuration_url} />
                    <Python path={python_url} />
                    <Building path={building_url} />
                    <Console path={console_url} />
                    <FirmwareFlasher path={flasher_url} />
                    <MidiPlayer path={midi_player_url} />
                    <Tools path={tools_url} />
                </Router>
            </main>
        </MDXProvider>
    );
}
