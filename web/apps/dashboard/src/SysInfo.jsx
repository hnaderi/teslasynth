import { useEffect, useState } from 'preact/hooks';
import { BooleanIndicator } from './components/BooleanIndicator';

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
                    <p>Detected system capabilities</p>
                </hgroup>
            </header>

            <div class="grid">
                <article class="syscard">
                    <header><strong>Firmware</strong></header>
                    <ul>
                        <li>Version: <code>{String(info['firmware']['version'])}</code></li>
                        <li>Compile time: <code>{String(info['firmware']['compile-time'])}</code></li>
                        <li>idf version: <code>{String(info['firmware']['idf-version'])}</code></li>
                    </ul>
                </article>

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
                        <li>Embedded Flash: <BooleanIndicator value={info['emb-flash']} label="emb-flash" /></li>
                    </ul>
                </article>

                <article class="syscard">
                    <header><strong>Connectivity</strong></header>
                    <ul>
                        <li>Wi-Fi: <BooleanIndicator value={info.wifi} label="Wi-Fi" /></li>
                        <li>BLE: <BooleanIndicator value={info.ble} label="BLE" /></li>
                        <li>Bluetooth: <BooleanIndicator value={info.bt} label="BT" /></li>
                        <li>USB-OTG: <BooleanIndicator value={info.otg} label="USB-OTG" /></li>
                    </ul>
                </article>
            </div>
        </article>
    );
}
