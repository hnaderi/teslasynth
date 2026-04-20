import { useEffect, useState } from 'preact/hooks';
import { InstallButton } from './components/InstallButton';
import { DocPage } from './Docs';
import intro from './content/intro.md?raw';

const CATALOG_BASE = 'https://cdn.jsdelivr.net/gh/hnaderi/teslasynth@firmware';

function LatestFirmwareInstall() {
    const BASE_URL = import.meta.env.BASE_URL;
    const flasher_url = BASE_URL + "/flasher";

    const [version, setVersion] = useState(null);
    const [manifest, setManifest] = useState(null);
    const [error, setError] = useState(null);

    useEffect(() => {
        if (!(navigator.serial || navigator.usb)) return;
        fetch(`${CATALOG_BASE}/catalog.json`)
            .then(r => r.json())
            .then(async versions => {
                const latest = versions[0];
                setVersion(latest);
                const r = await fetch(`${CATALOG_BASE}/${latest}/manifest.json`);
                return r.json();
            })
            .then(m => setManifest(m))
            .catch(e => setError(e.message));
    }, []);

    if (!(navigator.serial || navigator.usb)) {
        return <small>Web installer available in Chrome or Edge.</small>;
    }

    if (error) return <small style="color: var(--pico-del-color)">Could not load firmware: {error}</small>;

    return (
        <div style="display:flex; align-items:center; gap:1rem; flex-wrap:wrap; margin-top:1rem">
            <InstallButton manifest={manifest} version={version} />
            <a href={flasher_url}><small>Install a different version</small></a>
        </div>
    );
}

export function Home({ path }) {
    return (
        <>
            <DocPage path={path} content={intro} />
            <LatestFirmwareInstall />
        </>
    );
}
