import { useEffect, useState } from 'preact/hooks';
import { InstallButton } from './InstallButton';

const CATALOG_BASE = 'https://cdn.jsdelivr.net/gh/hnaderi/teslasynth@firmware';

export function LatestFirmwareInstall() {
    const BASE_URL = import.meta.env.BASE_URL;
    const flasher_url = BASE_URL + '/flasher';

    const [version, setVersion] = useState(null);
    const [manifest, setManifest] = useState(null);
    const [error, setError] = useState(null);

    useEffect(() => {
        if (!(navigator.serial || navigator.usb)) return;
        fetch(`${CATALOG_BASE}/catalog.json`)
            .then((r) => r.json())
            .then(async (versions) => {
                const latest = versions[0];
                setVersion(latest);
                const r = await fetch(
                    `${CATALOG_BASE}/${latest}/manifest.json`
                );
                return r.json();
            })
            .then((m) => setManifest(m))
            .catch((e) => setError(e.message));
    }, []);

    if (!(navigator.serial || navigator.usb)) {
        return (
            <p>
                <small>Web installer available in Chrome or Edge.</small>
            </p>
        );
    }

    if (error)
        return (
            <p>
                <small style="color: var(--pico-del-color)">
                    Could not load firmware: {error}
                </small>
            </p>
        );

    return (
        <div class="install-hero">
            <InstallButton
                manifest={manifest}
                version={version}
                label="Install firmware"
            />
            <a href={flasher_url}>
                <small>Install a different version</small>
            </a>
        </div>
    );
}
