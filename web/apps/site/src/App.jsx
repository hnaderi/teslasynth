/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

import { useEffect } from 'preact/hooks';
import { Logo } from '@teslasynth/ui/components/Logo';
import { Router, route } from 'preact-router';
import { MDXProvider } from '@mdx-js/preact';
import { Home } from './Home';
import GettingStarted from './content/getting_started.mdx';
import Midi from './content/midi.mdx';
import Configuration from './content/configuration.mdx';
import Python from './content/python.mdx';
import Building from './content/building.mdx';
import { Console } from './WebConsole';
import { FirmwareFlasher } from './Flasher';

export default function App() {
    const BASE_URL = import.meta.env.BASE_URL;
    const home_url = BASE_URL;
    const getting_started_url = BASE_URL + 'getting-started';
    const midi_url = BASE_URL + 'midi';
    const configuration_url = BASE_URL + 'configuration';
    const python_url = BASE_URL + 'python';
    const building_url = BASE_URL + 'building';
    const console_url = BASE_URL + 'console';
    const flasher_url = BASE_URL + 'flasher';

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
                        <a href={getting_started_url}>Getting started</a>
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
                        <a href={console_url}>Console</a>
                    </li>
                    <li>
                        <a
                            href="https://github.com/hnaderi/teslasynth"
                            target="_blank"
                        >
                            GitHub
                        </a>
                    </li>
                </ul>
                <details class="nav-mobile">
                    <summary>Menu</summary>
                    <ul>
                        <li>
                            <a href={home_url}>Home</a>
                        </li>
                        <li>
                            <a href={getting_started_url}>Getting started</a>
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
                            <a href={console_url}>Console</a>
                        </li>
                        <li>
                            <a
                                href="https://github.com/hnaderi/teslasynth"
                                target="_blank"
                            >
                                GitHub
                            </a>
                        </li>
                    </ul>
                </details>
            </nav>

            <main class="container">
                <Router>
                    <Home path={home_url} />
                    <GettingStarted path={getting_started_url} />
                    <Midi path={midi_url} />
                    <Configuration path={configuration_url} />
                    <Python path={python_url} />
                    <Building path={building_url} />
                    <Console path={console_url} />
                    <FirmwareFlasher path={flasher_url} />
                </Router>
            </main>
        </MDXProvider>
    );
}
