import { useEffect, useState, useRef } from 'preact/hooks';
import 'esp-web-tools/dist/install-button.js';

const CATALOG_BASE = 'https://cdn.jsdelivr.net/gh/hnaderi/teslasynth@firmware';

const CHIP_FAMILY = {
    esp32:   'ESP32',
    esp32s2: 'ESP32-S2',
    esp32s3: 'ESP32-S3',
};

export function toEspWebToolsManifest(manifest, version) {
    return {
        name: manifest.name,
        version,
        new_install_prompt_erase: true,
        builds: Object.values(manifest.targets).map(target => ({
            chipFamily: CHIP_FAMILY[target.extra_esptool_args?.chip] ?? target.extra_esptool_args?.chip,
            parts: target.files.map(f => ({
                path: `${CATALOG_BASE}/${version}/${f.path}`,
                offset: parseInt(f.offset, 16),
            })),
        })),
    };
}

/**
 * Renders an <esp-web-install-button> for a given manifest + version.
 * Handles blob URL creation and cleanup automatically.
 */
export function InstallButton({ manifest, version, label, disabled }) {
    const [manifestUrl, setManifestUrl] = useState(null);
    const prevUrlRef = useRef(null);

    useEffect(() => {
        if (!manifest || !version) return;

        const blob = new Blob([JSON.stringify(toEspWebToolsManifest(manifest, version))], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        setManifestUrl(url);

        if (prevUrlRef.current) URL.revokeObjectURL(prevUrlRef.current);
        prevUrlRef.current = url;

        return () => {
            URL.revokeObjectURL(url);
            prevUrlRef.current = null;
        };
    }, [manifest, version]);

    if (!manifestUrl) return null;

    return (
        <esp-web-install-button manifest={manifestUrl}>
            <button slot="activate" disabled={disabled}>
                {label ?? `Install ${version}`}
            </button>
        </esp-web-install-button>
    );
}
