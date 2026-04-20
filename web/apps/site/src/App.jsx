import { useEffect } from "preact/hooks";
import { Logo } from "@teslasynth/ui/components/Logo";
import { Router, route } from 'preact-router';
import { Home } from './Home';
import { DocPage } from './Docs';
import { Console } from "./WebConsole";
import { FirmwareFlasher } from "./Flasher";

import getting_started from './content/getting_started.md?raw';

export default function App() {
    const BASE_URL = import.meta.env.BASE_URL
    const home_url = BASE_URL
    const getting_started_url = BASE_URL + "/getting-started"
    const console_url = BASE_URL + "/console"
    const flasher_url = BASE_URL + "/flasher"

    useEffect(() => {
        const params = new URLSearchParams(window.location.search);
        const redirected = params.get('p');

        if (redirected) {
            history.replaceState(null, '', import.meta.env.BASE_URL);
            route(redirected, true);
        }
    }, []);

    return (
        <>
            <nav>
                <ul>
                    <Logo size={100} />
                </ul>
                <ul>
                    <li><a href={home_url}>Home</a></li>
                    <li><a href={getting_started_url}>Getting started</a></li>
                    <li><a href={console_url}>Console</a></li>
                </ul>
            </nav>

            <main class="container">
                <Router>
                    <Home path={home_url} />
                    <DocPage path={getting_started_url} content={getting_started} />
                    <Console path={console_url} />
                    <FirmwareFlasher path={flasher_url} />
                </Router>
            </main>
        </>
    );
}
