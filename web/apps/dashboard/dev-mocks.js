/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 *
 * Vite dev-only middleware that mocks the firmware HTTP API so the dashboard
 * can be developed without flashing an MCU. Active during `pnpm dev:dashboard`
 * only — production builds never include this file.
 *
 * Schemas mirror lib/app/web/server.cpp and lib/app/configuration/codec.cpp.
 */

const OUTPUTS = 4;
const MIDI_CHANNELS = 16;

const synthConfig = {
    tuning: 440,
    instrument: null,
    channels: Array.from({ length: OUTPUTS }, () => ({
        notes: 4,
        'max-on-time': 100,
        'min-deadtime': 100,
        'max-duty': 5.0,
        'duty-window': 10000,
        'pulse-resolution': 5,
        instrument: null,
    })),
    routing: {
        percussion: false,
        mapping: [0, 1, ...Array(MIDI_CHANNELS - 2).fill(-1)],
    },
};

const hardwareConfig = {
    output: { channels: [4, 5, 6, 7] },
    input: { pin: 0 },
    led: { pin: 8, 'active-high': true },
};

const sysInfo = {
    model: 1,
    cores: 2,
    'flash-size': 4 * 1024 * 1024,
    revision: 100,
    wifi: true,
    ble: true,
    bt: false,
    otg: false,
    'emb-flash': true,
    firmware: {
        version: 'dev-mock',
        'compile-time': new Date().toISOString().slice(0, 10),
        'idf-version': 'v5.4.1',
    },
};

const instruments = [
    'Square Wave',
    'Mono Lead',
    'Soft Lead',
    'Bright Lead',
    'Sync Lead',
    'Saw Lead',
    'Synth Pluck',
    'Harp Pluck',
    'Electric Pluck',
    'Flute',
    'Bell Key',
    'Sub Bass',
    'Analog Bass',
    'Rubber Bass',
    'Slap Bass',
    'Warm Pad',
    'Slow Pad',
    'Choir Pad',
    'Glass Pad',
    'Motion Pad',
    'Organ',
    'Brass',
    'Soft Brass',
    'Strings',
    'Staccato Strings',
    'Synth Hit',
    'Ping',
    'Rise FX',
    'Fall FX',
];

// In-memory state so PUT/DELETE behave like the real firmware: changes persist
// until vite restarts.
let liveSynth = synthConfig;
let liveHardware = hardwareConfig;

function readJsonBody(req) {
    return new Promise((resolve, reject) => {
        let raw = '';
        req.on('data', (chunk) => (raw += chunk));
        req.on('end', () => {
            try {
                resolve(raw ? JSON.parse(raw) : {});
            } catch (e) {
                reject(e);
            }
        });
        req.on('error', reject);
    });
}

function send(res, status, body) {
    res.statusCode = status;
    res.setHeader('Content-Type', 'application/json');
    res.end(typeof body === 'string' ? body : JSON.stringify(body));
}

export function mockApi() {
    return {
        name: 'teslasynth-dev-mocks',
        configureServer(server) {
            server.middlewares.use(async (req, res, next) => {
                if (!req.url.startsWith('/api/')) return next();

                try {
                    if (req.url === '/api/sys/info' && req.method === 'GET') {
                        return send(res, 200, sysInfo);
                    }

                    if (
                        req.url === '/api/sys/reboot' &&
                        req.method === 'POST'
                    ) {
                        return send(res, 200, {});
                    }

                    if (
                        req.url === '/api/synth/instruments' &&
                        req.method === 'GET'
                    ) {
                        return send(res, 200, instruments);
                    }

                    if (req.url === '/api/config/synth') {
                        if (req.method === 'GET')
                            return send(res, 200, liveSynth);
                        if (req.method === 'PUT') {
                            liveSynth = await readJsonBody(req);
                            return send(res, 200, liveSynth);
                        }
                        if (req.method === 'DELETE') {
                            liveSynth = synthConfig;
                            return send(res, 200, liveSynth);
                        }
                    }

                    if (req.url === '/api/config/hardware') {
                        if (req.method === 'GET')
                            return send(res, 200, liveHardware);
                        if (req.method === 'PUT') {
                            liveHardware = await readJsonBody(req);
                            return send(res, 200, liveHardware);
                        }
                        if (req.method === 'DELETE') {
                            liveHardware = hardwareConfig;
                            return send(res, 200, liveHardware);
                        }
                    }

                    return send(res, 404, {
                        error: 'mock not implemented',
                        url: req.url,
                    });
                } catch (err) {
                    return send(res, 500, { error: String(err) });
                }
            });
        },
    };
}
