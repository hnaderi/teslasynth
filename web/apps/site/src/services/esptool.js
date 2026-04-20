import { Transport } from 'esptool-js';
import { serial } from 'web-serial-polyfill';

function getSerialLib() {
    return !navigator.serial && navigator.usb ? serial : navigator.serial;
}

export async function openSerialPort() {
    return await getSerialLib().requestPort();
}

export async function openEspTransport() {
    return new Transport(await openSerialPort());
}
