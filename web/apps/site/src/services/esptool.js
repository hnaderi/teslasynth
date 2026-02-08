import { ESPLoader, Transport } from 'esptool-js';
import { serial } from 'web-serial-polyfill';

export async function openEspTransport() {
    const serialLib = !navigator.serial && navigator.usb ? serial : navigator.serial;
    const port = await serialLib.requestPort();
    const transport = new Transport(port);
    return transport;
}

export async function flashFirmware({
    firmware,
    transport,
    log
}) {

    const loader = new ESPLoader({
        transport,
        baudrate: firmware.baud,
        terminal: {
            writeLine: log,
            write: log
        }
    });

    await loader.main();

    const files = await Promise.all(
        firmware.files.map(async f => ({
            data: await fetch(f.url).then(r => r.arrayBuffer()),
            address: f.offset
        }))
    );

    await loader.writeFlash({
        fileArray: files,
        flashSize: 'keep'
    });

    await transport.disconnect();
}

