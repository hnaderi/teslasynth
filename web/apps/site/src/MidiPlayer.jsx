/**
 * Copyright Hossein Naderi 2025, 2026
 * SPDX-License-Identifier: GPL-3.0-only
 */

import { useState, useEffect, useRef } from 'preact/hooks';
import { parseMidi } from './lib/midiParser';
import { createMidiEngine } from './lib/midiEngine';
import { INSTRUMENTS } from './lib/instruments';
import './MidiPlayer.scss';

function fmtTime(secs) {
    const m = Math.floor(secs / 60);
    const s = (secs % 60).toFixed(1).padStart(4, '0');
    return `${m}:${s}`;
}

export function MidiPlayer({ collapsible = false }) {
    const midiAccessRef = useRef(null);
    const [midiStatus, setMidiStatus] = useState('—');
    const [ports, setPorts] = useState([]);
    const [selectedPortId, setSelectedPortId] = useState('');

    const [midiFile, setMidiFile] = useState(null);
    const [fileName, setFileName] = useState('');
    const [isDragging, setIsDragging] = useState(false);

    const [isPlaying, setIsPlaying] = useState(false);
    const [loop, setLoop] = useState(false);

    const [volume, setVolume] = useState(100);
    const [tempoScale, setTempoScale] = useState(100);
    const [transpose, setTranspose] = useState(0);
    const [mutedTracks, setMutedTracks] = useState(new Set());
    const [channelPrograms, setChannelPrograms] = useState({});

    const [engineMode, setEngineMode] = useState('—');
    const [stats, setStats] = useState({ sent: 0, dropped: 0 });

    // Start collapsed when used inside the Tools page
    const [collapsed, setCollapsed] = useState(
        () => collapsible && localStorage.getItem('tool:midi') !== 'open'
    );

    const seekRef = useRef(null);
    const timeRef = useRef(null);
    const trackLedRefs = useRef([]);
    const activityTimesRef = useRef([]);
    const seekingRef = useRef(false);
    const engineRef = useRef(null);
    const midiFileRef = useRef(null);
    const onEventRef = useRef(null);
    const rafRef = useRef(null);

    onEventRef.current = (type, data) => {
        if (type === 'play') setIsPlaying(true);
        else if (type === 'pause' || type === 'stop' || type === 'complete')
            setIsPlaying(false);
        else if (type === 'mode') setEngineMode(data);
        else if (type === 'stats')
            setStats({ sent: data.sent, dropped: data.dropped });
    };

    useEffect(() => {
        const engine = createMidiEngine({
            onEvent: (type, data) => onEventRef.current(type, data),
            onActivity: (idx) => {
                activityTimesRef.current[idx] = performance.now();
            },
        });
        engineRef.current = engine;
        const safeStop = () => engine.stop();
        window.addEventListener('beforeunload', safeStop);
        return () => {
            window.removeEventListener('beforeunload', safeStop);
            engine.dispose();
        };
    }, []);

    // Defer MIDI access request until the player is actually opened
    useEffect(() => {
        if (collapsed) return;
        if (midiAccessRef.current) return; // already granted
        if (!navigator.requestMIDIAccess) {
            setMidiStatus('N/A');
            return;
        }
        navigator
            .requestMIDIAccess({ sysex: false })
            .then((access) => {
                midiAccessRef.current = access;
                setMidiStatus('OK');
                access.onstatechange = refreshPorts;
                refreshPorts();
            })
            .catch(() => setMidiStatus('DENIED'));
    }, [collapsed]);

    useEffect(() => {
        const onKey = (e) => {
            if (e.target.tagName === 'INPUT' || e.target.tagName === 'SELECT')
                return;
            const eng = engineRef.current;
            if (!eng) return;
            if (e.code === 'Space') {
                e.preventDefault();
                eng.play();
            } else if (e.code === 'Escape') {
                e.preventDefault();
                eng.stop();
            } else if (e.code === 'ArrowLeft') {
                e.preventDefault();
                handleRewind();
            }
        };
        window.addEventListener('keydown', onKey);
        return () => window.removeEventListener('keydown', onKey);
    }, []);

    useEffect(() => {
        function frame() {
            const eng = engineRef.current;
            const file = midiFileRef.current;
            if (!eng?.isPlayingNow() || !file) {
                rafRef.current = null;
                return;
            }
            const pos = eng.getPositionUs();
            if (!seekingRef.current && seekRef.current)
                seekRef.current.value = (pos / file.endUs) * 1000;
            if (timeRef.current)
                timeRef.current.textContent = fmtTime(pos / 1e6);
            const now = performance.now();
            trackLedRefs.current.forEach((el, i) => {
                if (!el) return;
                const active = now - (activityTimesRef.current[i] ?? 0) < 150;
                el.style.background = active
                    ? 'var(--pico-primary)'
                    : 'var(--pico-muted-border-color)';
            });
            rafRef.current = requestAnimationFrame(frame);
        }
        if (isPlaying && !rafRef.current)
            rafRef.current = requestAnimationFrame(frame);
    }, [isPlaying]);

    function onToggle(e) {
        const isOpen = e.currentTarget.open;
        if (isOpen === !collapsed) return;
        setCollapsed(!isOpen);
        if (collapsible)
            localStorage.setItem('tool:midi', isOpen ? 'open' : 'closed');
    }

    function refreshPorts() {
        const access = midiAccessRef.current;
        if (!access) return;
        const list = [];
        access.outputs.forEach((port, id) =>
            list.push({ id, name: port.name })
        );
        setPorts(list);
        setSelectedPortId((prev) => {
            if (prev && !access.outputs.has(prev)) {
                engineRef.current?.setMidiOut(null);
                return '';
            }
            return prev;
        });
    }

    function loadFile(file) {
        const reader = new FileReader();
        reader.onload = (e) => {
            try {
                const parsed = parseMidi(e.target.result);
                midiFileRef.current = parsed;
                setMidiFile(parsed);
                setFileName(file.name);
                setMutedTracks(new Set());
                setChannelPrograms({});
                activityTimesRef.current = new Array(
                    parsed.trackNames.length
                ).fill(0);
                trackLedRefs.current = new Array(parsed.trackNames.length).fill(
                    null
                );
                if (seekRef.current) seekRef.current.value = 0;
                if (timeRef.current) timeRef.current.textContent = fmtTime(0);
                engineRef.current?.load(parsed);
            } catch (err) {
                alert(`MIDI parse error: ${err.message}`);
            }
        };
        reader.readAsArrayBuffer(file);
    }

    function selectPort(id) {
        setSelectedPortId(id);
        const port = id ? midiAccessRef.current?.outputs.get(id) : null;
        engineRef.current?.setMidiOut(port ?? null);
    }

    function handleRewind() {
        const eng = engineRef.current;
        if (!eng) return;
        const wasPlaying = eng.isPlayingNow();
        eng.stop();
        if (wasPlaying) eng.play();
    }

    function handleLoopToggle() {
        setLoop((prev) => {
            engineRef.current?.setLoop(!prev);
            return !prev;
        });
    }

    function handleChannelProgramChange(ch, program) {
        setChannelPrograms((prev) => ({ ...prev, [ch]: program }));
        engineRef.current?.setChannelProgram(ch, program);
    }

    function toggleMute(i) {
        setMutedTracks((prev) => {
            const next = new Set(prev);
            const nowMuted = next.has(i)
                ? (next.delete(i), false)
                : (next.add(i), true);
            engineRef.current?.muteTrack(i, nowMuted);
            return next;
        });
    }

    const hasFile = !!midiFile;
    const eng = engineRef.current;
    const statusClass =
        midiStatus === 'OK'
            ? 'midi-player__status--ok'
            : midiStatus === '—'
              ? ''
              : 'midi-player__status--err';

    const statusLine = (
        <small class="midi-player__status">
            MIDI&nbsp;<strong class={statusClass}>{midiStatus}</strong>
            {' · '}Engine&nbsp;<strong>{engineMode}</strong>
            {stats.sent > 0 && <> · {stats.sent}&nbsp;sent</>}
            {stats.dropped > 0 && (
                <>
                    {' '}
                    ·{' '}
                    <span class="midi-player__status--err">
                        {stats.dropped}&nbsp;dropped
                    </span>
                </>
            )}
        </small>
    );

    const body = (
        <>
            <div
                class={`midi-player__drop-zone${isDragging ? ' midi-player__drop-zone--over' : ''}`}
                onDragOver={(e) => {
                    e.preventDefault();
                    setIsDragging(true);
                }}
                onDragLeave={() => setIsDragging(false)}
                onDrop={(e) => {
                    e.preventDefault();
                    setIsDragging(false);
                    const f = e.dataTransfer.files[0];
                    if (f) loadFile(f);
                }}
                onClick={() =>
                    document.getElementById('_midi_file_input').click()
                }
            >
                <input
                    id="_midi_file_input"
                    type="file"
                    accept=".mid,.midi"
                    style="display:none"
                    onChange={(e) => {
                        if (e.target.files[0]) loadFile(e.target.files[0]);
                    }}
                />
                {fileName ? (
                    <>
                        <strong>{fileName}</strong>
                        <br />
                        <small>
                            Format&nbsp;{midiFile.format}
                            &ensp;·&ensp;{midiFile.nTracks}&nbsp;track
                            {midiFile.nTracks !== 1 ? 's' : ''}
                            &ensp;·&ensp;{midiFile.initBPM}&nbsp;BPM
                            &ensp;·&ensp;{fmtTime(midiFile.endUs / 1e6)}
                        </small>
                    </>
                ) : (
                    <span class="hint">
                        Drop a <code>.mid</code> / <code>.midi</code> file here,
                        or click to browse
                    </span>
                )}
            </div>

            <label>
                MIDI Output
                <select
                    value={selectedPortId}
                    onChange={(e) => selectPort(e.target.value)}
                    disabled={ports.length === 0}
                >
                    <option value="">
                        {ports.length === 0
                            ? 'No MIDI outputs detected'
                            : '— Select output —'}
                    </option>
                    {ports.map((p) => (
                        <option key={p.id} value={p.id}>
                            {p.name}
                        </option>
                    ))}
                </select>
            </label>

            <div class="midi-player__transport">
                <button
                    onClick={() => eng?.play()}
                    disabled={!hasFile}
                    class={`midi-player__play-btn${isPlaying ? '' : ' secondary'}`}
                >
                    {isPlaying ? '⏸ Pause' : '▶ Play'}
                </button>
                <button
                    onClick={() => eng?.stop()}
                    disabled={!hasFile}
                    class="secondary"
                >
                    ■ Stop
                </button>
                <button
                    onClick={handleRewind}
                    disabled={!hasFile}
                    class="secondary"
                >
                    ⏮ Rewind
                </button>
                <button
                    onClick={handleLoopToggle}
                    disabled={!hasFile}
                    class={loop ? '' : 'secondary'}
                >
                    ↻ Loop
                </button>
                <button
                    onClick={() => eng?.allNotesOff()}
                    class="secondary outline"
                >
                    ✕ Notes Off
                </button>
            </div>

            <div class="midi-player__seek">
                <input
                    type="range"
                    ref={seekRef}
                    min="0"
                    max="1000"
                    defaultValue="0"
                    disabled={!hasFile}
                    onMouseDown={() => {
                        seekingRef.current = true;
                    }}
                    onTouchStart={() => {
                        seekingRef.current = true;
                    }}
                    onMouseUp={(e) => {
                        seekingRef.current = false;
                        eng?.seek(e.target.value / 1000);
                    }}
                    onTouchEnd={(e) => {
                        seekingRef.current = false;
                        eng?.seek(e.target.value / 1000);
                    }}
                />
                <code ref={timeRef}>{fmtTime(0)}</code>
                <code class="total">
                    / {midiFile ? fmtTime(midiFile.endUs / 1e6) : fmtTime(0)}
                </code>
            </div>

            <div class="grid">
                <label>
                    Volume&ensp;<small>{volume}</small>
                    <input
                        type="range"
                        min="0"
                        max="127"
                        value={volume}
                        onInput={(e) => {
                            const v = +e.target.value;
                            setVolume(v);
                            eng?.setVolume(v);
                        }}
                    />
                </label>
                <label>
                    Tempo&ensp;<small>{tempoScale}%</small>
                    <input
                        type="range"
                        min="25"
                        max="200"
                        value={tempoScale}
                        onInput={(e) => {
                            const v = +e.target.value;
                            setTempoScale(v);
                            eng?.setTempoScale(v / 100);
                        }}
                    />
                </label>
                <label>
                    Transpose&ensp;
                    <small>
                        {transpose > 0 ? '+' : ''}
                        {transpose}&nbsp;st
                    </small>
                    <input
                        type="range"
                        min="-12"
                        max="12"
                        value={transpose}
                        onInput={(e) => {
                            const v = +e.target.value;
                            setTranspose(v);
                            eng?.setTranspose(v);
                        }}
                    />
                </label>
            </div>

            {midiFile && (
                <>
                    <fieldset>
                        <legend>
                            <strong>Tracks</strong>
                        </legend>
                        {midiFile.trackNames.map((name, i) => {
                            if (midiFile.trackChannels[i].length === 0)
                                return null;
                            return (
                                <div key={i} class="midi-player__track">
                                    <input
                                        type="checkbox"
                                        role="switch"
                                        checked={!mutedTracks.has(i)}
                                        onChange={() => toggleMute(i)}
                                    />
                                    <span
                                        ref={(el) => {
                                            trackLedRefs.current[i] = el;
                                        }}
                                        class="midi-player__track-led"
                                    />
                                    <span class="midi-player__track-name">
                                        {name}
                                    </span>
                                    <small class="midi-player__track-ch">
                                        CH&nbsp;
                                        {midiFile.trackChannels[i]
                                            .map((c) => c + 1)
                                            .join(', ')}
                                    </small>
                                </div>
                            );
                        })}
                    </fieldset>

                    {(() => {
                        const channels = [
                            ...new Set(midiFile.trackChannels.flat()),
                        ]
                            .filter((ch) => ch !== 9)
                            .sort((a, b) => a - b);
                        if (channels.length === 0) return null;
                        return (
                            <fieldset>
                                <legend>
                                    <strong>Instruments</strong>
                                </legend>
                                {channels.map((ch) => (
                                    <div
                                        key={ch}
                                        class="midi-player__channel-row"
                                    >
                                        <small class="midi-player__track-ch">
                                            CH&nbsp;{ch + 1}
                                        </small>
                                        <select
                                            value={channelPrograms[ch] ?? -1}
                                            onChange={(e) =>
                                                handleChannelProgramChange(
                                                    ch,
                                                    +e.target.value
                                                )
                                            }
                                        >
                                            <option value="-1">
                                                Auto (from file)
                                            </option>
                                            {INSTRUMENTS.map((name, idx) => (
                                                <option key={idx} value={idx}>
                                                    {idx}&nbsp;–&nbsp;{name}
                                                </option>
                                            ))}
                                        </select>
                                    </div>
                                ))}
                            </fieldset>
                        );
                    })()}
                </>
            )}

            <small class="midi-player__footer">
                Space = play/pause · Esc = stop · ← = rewind
            </small>
        </>
    );

    if (collapsible) {
        return (
            <article>
                <details open={!collapsed} onToggle={onToggle}>
                    <summary class="midi-player__summary">
                        MIDI Player
                        {!collapsed && statusLine}
                    </summary>
                    {body}
                </details>
            </article>
        );
    }

    return (
        <article>
            <header>
                <div class="midi-player__header">
                    <h2>MIDI Player</h2>
                    <div class="midi-player__header-end">{statusLine}</div>
                </div>
            </header>
            {body}
        </article>
    );
}
