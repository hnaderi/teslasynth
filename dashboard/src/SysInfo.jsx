import { useEffect, useState } from 'preact/hooks';

export function SysInfoSection() {
    const [info, setInfo] = useState(null);

    useEffect(() => {
        fetch('/api/sys/info')
            .then(r => r.json())
            .then(setInfo);
    }, []);

    if (!info) return <article ><header aria-busy="true">Loading System Info…</header></article>;

    return (
        <article>
            <header>
                <hgroup>
                    <h2>System Information</h2>
                    <p>Detected hardware capabilities</p>
                </hgroup>
            </header>

            <div class="grid">
                <article class="syscard">
                    <header><strong>Device</strong></header>
                    <ul>
                        <li>Model: <code>{info.model}</code></li>
                        <li>Revision: <code>{info.revision}</code></li>
                    </ul>
                </article>

                <article class="syscard">
                    <header><strong>Resources</strong></header>
                    <ul>
                        <li>Cores: <code>{info.cores}</code></li>
                        <li>Flash Size: <code>{info['flash-size']} MB</code></li>
                        <li>Embedded Flash: <code>{String(info['emb-flash'])}</code></li>
                    </ul>
                </article>

                <article class="syscard">
                    <header><strong>Connectivity</strong></header>
                    <ul>
                        <li>Wi-Fi: <code>{String(info.wifi)}</code></li>
                        <li>BLE: <code>{String(info.ble)}</code></li>
                        <li>Bluetooth: <code>{String(info.bt)}</code></li>
                    </ul>
                </article>
            </div>
        </article>
    );
}
