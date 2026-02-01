import { useEffect } from "preact/hooks";
import { Logo } from "@teslasynth/ui/components/Logo";
import { Router, route } from 'preact-router';
import { DocPage } from './Docs';

import intro from './content/intro.md?raw';
import getting_started from './content/getting_started.md?raw';

export default function App() {
    const BASE_URL = import.meta.env.BASE_URL
    const home_url = BASE_URL
    const getting_started_url = BASE_URL + "/getting-started"
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
                </ul>
            </nav>

            <Router>
                <DocPage path={home_url} content={intro} />
                <DocPage path={getting_started_url} content={getting_started} />
            </Router>

        </>
    );
}
