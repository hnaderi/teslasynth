import { useEffect, useState } from 'preact/hooks';
import { InstallButton } from './components/InstallButton';

const CATALOG_BASE = 'https://cdn.jsdelivr.net/gh/hnaderi/teslasynth@firmware';
const FIRST_SUPPORTED_VERSION = 'v0.4.1';

export function FirmwareFlasher() {
    const [versions, setVersions] = useState([]);
    const [selectedVersion, setSelectedVersion] = useState(null);
    const [manifest, setManifest] = useState(null);
    const [loadingManifest, setLoadingManifest] = useState(false);
    const [error, setError] = useState(null);

    useEffect(() => {
        fetch(`${CATALOG_BASE}/catalog.json`)
            .then(r => r.json())
            .then(data => {
                const idx = data.indexOf(FIRST_SUPPORTED_VERSION);
                const supported = idx !== -1 ? data.slice(0, idx + 1) : data;
                setVersions(supported);
                if (supported.length > 0) setSelectedVersion(supported[0]);
            })
            .catch(e => setError(`Failed to load firmware catalog: ${e.message}`));
    }, []);

    useEffect(() => {
        if (!selectedVersion) return;
        setLoadingManifest(true);
        setManifest(null);
        fetch(`${CATALOG_BASE}/${selectedVersion}/manifest.json`)
            .then(r => r.json())
            .then(m => setManifest(m))
            .catch(e => setError(`Failed to load manifest for ${selectedVersion}: ${e.message}`))
            .finally(() => setLoadingManifest(false));
    }, [selectedVersion]);

    if (!(navigator.serial || navigator.usb)) {
        return <article><p>Web Serial is not supported in this browser. Please use Chrome or Edge.</p></article>;
    }

    return (
        <article>
            <header><h2>Install firmware</h2></header>

            {error && <p style="color: var(--pico-del-color)">{error}</p>}

            <label>
                Version
                <select
                    value={selectedVersion || ''}
                    onChange={e => setSelectedVersion(e.target.value)}
                    disabled={versions.length === 0}
                >
                    {versions.length === 0
                        ? <option>Loading...</option>
                        : versions.map(v => <option key={v} value={v}>{v}</option>)
                    }
                </select>
            </label>

            <InstallButton
                manifest={manifest}
                version={selectedVersion}
                label={loadingManifest ? 'Loading...' : 'Connect & Flash'}
                disabled={loadingManifest}
            />
        </article>
    );
}
