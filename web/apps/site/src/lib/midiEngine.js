/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

// Tiny worker used as a throttle-immune scheduling timer.
// Main-thread setTimeout is throttled to ≥1 s in background tabs;
// Worker timers are not subject to that restriction.
const TIMER_WORKER_SRC = `
let timer = null;
self.onmessage = ({ data }) => {
    if (data === 'start' && !timer)
        timer = setInterval(() => self.postMessage('tick'), 200);
    else if (data === 'stop') { clearInterval(timer); timer = null; }
};
`;

// Ring buffer constants (SAB layout):
//   [0] write head, [1] read head, then RING_SLOTS * 5 int32 slots
//   each slot: [ usHi, usLo, status, b1, b2 ]
const RING_SLOTS = 4096;
const SAB_INTS = 2 + RING_SLOTS * 5;

const WORKLET_SRC = `
class MidiSchedulerProcessor extends AudioWorkletProcessor {
  constructor(opts) {
    super();
    const sab = opts.processorOptions.sab;
    this._ring = sab ? new Int32Array(sab) : null;
    this._queue = [];
    this.port.onmessage = ({ data }) => {
      if (data.type === 'event') {
        if (this._ring) {
          const wr = Atomics.load(this._ring, 0);
          const s = 2 + (wr % 4096) * 5;
          this._ring[s]   = Math.floor(data.us / 0x100000000);
          this._ring[s+1] = data.us >>> 0;
          this._ring[s+2] = data.st;
          this._ring[s+3] = data.b1;
          this._ring[s+4] = data.b2;
          Atomics.store(this._ring, 0, (wr + 1) % 4096);
        } else {
          this._queue.push(data);
          this._queue.sort((a, b) => a.us - b.us);
        }
      } else if (data.type === 'flush') {
        if (this._ring) {
          Atomics.store(this._ring, 0, 0);
          Atomics.store(this._ring, 1, 0);
        } else {
          this._queue = [];
        }
      }
    };
  }
  process() {
    const nowUs = currentTime * 1e6;
    const winUs = nowUs + 3000;
    if (this._ring) {
      let rd = Atomics.load(this._ring, 1);
      const wr = Atomics.load(this._ring, 0);
      while (rd !== wr) {
        const s = 2 + rd * 5;
        const evUs = this._ring[s] * 0x100000000 + (this._ring[s+1] >>> 0);
        if (evUs > winUs) break;
        this.port.postMessage({ type: 'fire', st: this._ring[s+2], b1: this._ring[s+3], b2: this._ring[s+4] });
        rd = (rd + 1) % 4096;
        Atomics.store(this._ring, 1, rd);
      }
    } else {
      while (this._queue.length && this._queue[0].us <= winUs) {
        const ev = this._queue.shift();
        this.port.postMessage({ type: 'fire', st: ev.st, b1: ev.b1, b2: ev.b2 });
      }
    }
    return true;
  }
}
registerProcessor('midi-scheduler', MidiSchedulerProcessor);
`;

/**
 * Creates a MIDI playback engine backed by an AudioWorklet scheduler.
 *
 * @param {object} callbacks
 * @param {(type: string, data: any) => void} callbacks.onEvent
 *   'play' | 'pause' | 'stop' | 'complete' | 'load' | 'mode' | 'stats'
 * @param {(trackIndex: number) => void} callbacks.onActivity
 *   Called when a note-on fires on a given track (for LED indicators).
 */
export function createMidiEngine({ onEvent, onActivity }) {
    let audioCtx = null;
    let workletNode = null;
    let midiOut = null;
    let file = null;

    let isPlaying = false;
    let loop = false;
    let offsetUs = 0;
    let startTime = 0;
    let nextIdx = 0;

    const mutedTracks = new Set();
    const programOverrides = new Map(); // ch → program number (0-based instrument id)
    let volume = 127;
    let tempoScale = 1.0;
    let transpose = 0;

    let timerWorker = null;
    let completionTimer = null;
    let sent = 0;
    let dropped = 0;
    let mode = '—';
    let chToTrack = new Map();

    // ── helpers ──────────────────────────────────────────────────────────────

    function getPositionUs() {
        if (!audioCtx || !isPlaying) return offsetUs;
        return (audioCtx.currentTime - startTime) * 1e6 * tempoScale + offsetUs;
    }

    function allNotesOff() {
        if (!midiOut) return;
        for (let ch = 0; ch < 16; ch++) {
            midiOut.send([0xb0 | ch, 123, 0]); // all-notes-off
            midiOut.send([0xb0 | ch, 120, 0]); // all-sound-off
        }
    }

    function flushWorklet() {
        timerWorker?.postMessage('stop');
        clearTimeout(completionTimer);
        if (workletNode) workletNode.port.postMessage({ type: 'flush' });
    }

    function sendVolume() {
        if (!midiOut) return;
        for (let ch = 0; ch < 16; ch++) midiOut.send([0xb0 | ch, 7, volume]);
    }

    function sendProgramOverrides() {
        if (!midiOut) return;
        programOverrides.forEach((program, ch) =>
            midiOut.send([0xc0 | ch, program])
        );
    }

    // Re-synchronise in-flight events after a parameter change mid-playback.
    function resync() {
        if (!isPlaying || !audioCtx) return;
        offsetUs = getPositionUs();
        flushWorklet();
        allNotesOff();
        startTime = audioCtx.currentTime;
        nextIdx = file.events.findIndex((e) => e.us >= offsetUs);
        if (nextIdx < 0) nextIdx = file.events.length;
        scheduleAhead();
    }

    // ── AudioWorklet setup ────────────────────────────────────────────────────

    async function ensureAudio() {
        if (audioCtx) return;
        audioCtx = new AudioContext({
            latencyHint: 'interactive',
            sampleRate: 44100,
        });

        const blob = new Blob([WORKLET_SRC], {
            type: 'application/javascript',
        });
        const url = URL.createObjectURL(blob);
        await audioCtx.audioWorklet.addModule(url);
        URL.revokeObjectURL(url);

        let sab = null;
        if (typeof SharedArrayBuffer !== 'undefined') {
            sab = new SharedArrayBuffer(SAB_INTS * 4);
            mode = 'SAB';
        } else {
            mode = 'postMessage';
        }
        onEvent('mode', mode);

        workletNode = new AudioWorkletNode(audioCtx, 'midi-scheduler', {
            processorOptions: { sab },
            numberOfInputs: 0,
            numberOfOutputs: 1,
            outputChannelCount: [1],
        });
        workletNode.port.onmessage = onWorkletFire;

        const gain = audioCtx.createGain();
        gain.gain.value = 0;
        workletNode.connect(gain);
        gain.connect(audioCtx.destination);
    }

    function onWorkletFire({ data }) {
        if (data.type !== 'fire') return;
        const { st, b1, b2 } = data;

        if ((st & 0xf0) === 0x90 && b2 > 0) {
            const tIdx = chToTrack.get(st & 0x0f);
            if (tIdx !== undefined) onActivity(tIdx);
        }

        if (!midiOut) {
            dropped++;
        } else {
            try {
                midiOut.send([st, b1, b2]);
                sent++;
            } catch {
                dropped++;
            }
        }
        onEvent('stats', { sent, dropped });
    }

    // ── Scheduler ─────────────────────────────────────────────────────────────

    function scheduleAhead() {
        if (!isPlaying || !workletNode) return;
        const nowUs = getPositionUs();
        const winUs = nowUs + 500000; // 500 ms lookahead

        while (nextIdx < file.events.length) {
            const ev = file.events[nextIdx];
            if (ev.us > winUs) break;

            if (ev.type === 'midi') {
                const tIdx = chToTrack.get(ev.ch) ?? -1;
                const pcOverridden =
                    ev.msgType === 0xc0 && programOverrides.has(ev.ch);
                if (!mutedTracks.has(tIdx) && !pcOverridden) {
                    let { b1, b2 } = ev;
                    if (
                        (ev.msgType === 0x90 || ev.msgType === 0x80) &&
                        transpose !== 0
                    ) {
                        b1 = Math.max(0, Math.min(127, b1 + transpose));
                    }
                    const fireUs =
                        (startTime + (ev.us - offsetUs) / (1e6 * tempoScale)) *
                        1e6;
                    workletNode.port.postMessage({
                        type: 'event',
                        us: fireUs,
                        st: ev.status,
                        b1,
                        b2,
                    });
                }
            }
            nextIdx++;
        }

        if (nextIdx >= file.events.length) {
            const lastUs = file.events[file.events.length - 1]?.us ?? 0;
            const delayMs = ((lastUs - nowUs) / 1e6 / tempoScale) * 1000 + 500;
            completionTimer = setTimeout(() => {
                if (!isPlaying) return;
                if (loop) {
                    offsetUs = 0;
                    startTime = audioCtx.currentTime;
                    nextIdx = 0;
                    allNotesOff();
                    scheduleAhead();
                } else {
                    stop();
                    onEvent('complete', null);
                }
            }, delayMs);
            return;
        }

        if (!timerWorker) {
            const blob = new Blob([TIMER_WORKER_SRC], {
                type: 'application/javascript',
            });
            timerWorker = new Worker(URL.createObjectURL(blob));
            timerWorker.onmessage = () => scheduleAhead();
        }
        timerWorker.postMessage('start');
    }

    // ── Public API ────────────────────────────────────────────────────────────

    async function play() {
        if (!file) return;
        if (isPlaying) {
            pause();
            return;
        }

        await ensureAudio();
        if (audioCtx.state === 'suspended') await audioCtx.resume();

        if (offsetUs >= file.endUs) offsetUs = 0;

        isPlaying = true;
        startTime = audioCtx.currentTime;
        nextIdx = file.events.findIndex((e) => e.us >= offsetUs);
        if (nextIdx < 0) nextIdx = file.events.length;

        scheduleAhead();
        sendVolume();
        sendProgramOverrides();
        onEvent('play', null);
    }

    function pause() {
        if (!isPlaying) return;
        isPlaying = false;
        offsetUs = getPositionUs();
        flushWorklet();
        allNotesOff();
        onEvent('pause', null);
    }

    function stop() {
        isPlaying = false;
        offsetUs = 0;
        flushWorklet();
        allNotesOff();
        onEvent('stop', null);
    }

    function seek(fraction) {
        if (!file) return;
        const wasPlaying = isPlaying;
        if (wasPlaying) {
            isPlaying = false;
            flushWorklet();
            allNotesOff();
        }
        offsetUs = Math.max(0, Math.min(file.endUs, fraction * file.endUs));
        if (wasPlaying) {
            isPlaying = true;
            startTime = audioCtx.currentTime;
            nextIdx = file.events.findIndex((e) => e.us >= offsetUs);
            if (nextIdx < 0) nextIdx = file.events.length;
            scheduleAhead();
        }
    }

    return {
        load(parsedFile) {
            file = parsedFile;
            chToTrack = new Map();
            parsedFile.trackChannels.forEach((chs, i) =>
                chs.forEach((ch) => chToTrack.set(ch, i))
            );
            programOverrides.clear();
            stop();
            sent = 0;
            dropped = 0;
            onEvent('load', parsedFile);
        },
        play,
        pause,
        stop,
        seek,

        allNotesOff,
        setMidiOut(port) {
            midiOut = port ?? null;
        },
        setVolume(v) {
            volume = Math.max(0, Math.min(127, v));
            sendVolume();
        },
        setTempoScale(scale) {
            tempoScale = Math.max(0.25, Math.min(2.0, scale));
            resync();
        },
        setTranspose(semitones) {
            transpose = Math.max(-12, Math.min(12, semitones));
            resync();
        },
        setLoop(v) {
            loop = v;
        },
        setChannelProgram(ch, program) {
            if (program < 0) {
                programOverrides.delete(ch);
            } else {
                programOverrides.set(ch, program);
                midiOut?.send([0xc0 | ch, program]);
            }
        },
        muteTrack(i, muted) {
            if (muted) mutedTracks.add(i);
            else mutedTracks.delete(i);
            resync();
        },

        getPositionUs,
        isPlayingNow: () => isPlaying,

        dispose() {
            stop();
            timerWorker?.terminate();
            timerWorker = null;
            audioCtx?.close();
            audioCtx = null;
            workletNode = null;
        },
    };
}
