"""
CLI entry point for the teslasynth package.

Usage
-----
    teslasynth render      <midi> <wav>  [--config FILE] [--sample-rate HZ] [--step-us US]
    teslasynth plot        <midi>        [--config FILE] [--out FILE.html] [--start-ms MS] [--end-ms MS]
    teslasynth signal      <midi>        [--config FILE] [--out FILE.html] [--start-ms MS] [--end-ms MS]
    teslasynth version
    teslasynth config      [--config FILE] [key=value ...]
    teslasynth instruments
    teslasynth envelope    <instrument|percussion>  [--out FILE.html] [--duration-ms MS]
"""
from __future__ import annotations

import argparse
import sys


def _die(msg: str) -> None:
    print(f"Error: {msg}", file=sys.stderr)
    sys.exit(1)


def _load_synth(config_path: str | None):
    """Return a configured Teslasynth instance (or default if no config given)."""
    from teslasynth import Teslasynth
    if config_path is None:
        return Teslasynth()
    from teslasynth import config as tscfg
    try:
        cfg = tscfg.load(config_path)
    except (OSError, ValueError) as exc:
        _die(str(exc))
    return Teslasynth(cfg)


def _find_instrument(name_or_id: str):
    """Resolve a name or integer index to an InstrumentId."""
    from teslasynth import get_all_instruments
    all_info = get_all_instruments()
    try:
        idx = int(name_or_id)
        if 0 <= idx < len(all_info):
            return all_info[idx].id
        _die(f"instrument index {idx} out of range (0–{len(all_info) - 1})")
    except ValueError:
        for info in all_info:
            if info.name.lower() == name_or_id.lower():
                return info.id
        names = "\n  ".join(f"{i.index:2d}  {i.name}" for i in all_info)
        _die(f"unknown instrument '{name_or_id}'. Valid names:\n  {names}")


def _find_instrument_or_percussion(name_or_id: str):
    """Resolve a name or index to an InstrumentId or PercussionId.

    Instruments are tried first; percussion names are matched case-insensitively.
    """
    from teslasynth import get_all_instruments, get_all_percussions
    instruments = get_all_instruments()
    percussions = get_all_percussions()
    try:
        idx = int(name_or_id)
        if 0 <= idx < len(instruments):
            return instruments[idx].id
        _die(f"instrument index {idx} out of range (0–{len(instruments) - 1})")
    except ValueError:
        for info in instruments:
            if info.name.lower() == name_or_id.lower():
                return info.id
        for info in percussions:
            if info.name.lower() == name_or_id.lower():
                return info.id
        inst_names = "\n  ".join(f"{i.index:2d}  {i.name}" for i in instruments)
        perc_names = "\n  ".join(f"{i.index:2d}  {i.name}" for i in percussions)
        _die(f"unknown instrument or percussion '{name_or_id}'.\n"
             f"Instruments:\n  {inst_names}\nPercussions:\n  {perc_names}")


_BANNER = """\
 _____         _                       _   _
|_   _|       | |                     | | | |
  | | ___  ___| | __ _ ___ _   _ _ __ | |_| |__
  | |/ _ \\/ __| |/ _` / __| | | | '_ \\| __| '_ \\
  | |  __/\\__ \\ | (_| \\__ \\ |_| | | | | |_| | | |
  \\_/\\___||___/_|\\__,_|___/\\__, |_| |_|\\__|_| |_|
                            __/ |
                           |___/\
"""


def _cmd_version(_args: argparse.Namespace) -> None:
    from teslasynth import build_info
    info = build_info()
    print(_BANNER)
    print(f"version:{info['version']}")
    print(f"compiled at:{info['date']} {info['time']}")


def _save_or_show(fig, out: str | None) -> None:
    if out:
        fig.write_html(out)
        print(f"Written: {out}")
    else:
        fig.show()


def _cmd_render(args: argparse.Namespace) -> None:
    from teslasynth import wav
    try:
        synth = _load_synth(args.config)
        print(f"Rendering {args.midi} → {args.wav} …", file=sys.stderr)
        wav.write(args.midi, args.wav,
                  synth=synth,
                  sample_rate=args.sample_rate,
                  step_us=args.step_us,
                  channel=args.channel)
        print(f"Written: {args.wav}")
    except FileNotFoundError as exc:
        _die(str(exc))


def _cmd_plot(args: argparse.Namespace) -> None:
    from teslasynth import render, plot
    from teslasynth import midi as tsm
    try:
        # Notes are cheap to extract; do it before the expensive synthesis.
        notes = tsm.notes_from_midi(args.midi)
        synth = _load_synth(args.config)
        print(f"Synthesising {args.midi} …", file=sys.stderr)
        rec = render.from_file(args.midi, synth=synth, channel=args.channel)
    except FileNotFoundError as exc:
        _die(str(exc))

    start_ms = args.start_ms if args.start_ms is not None else 0.0
    fig = plot.plot_overview(rec, notes=notes, start_ms=start_ms, end_ms=args.end_ms)
    _save_or_show(fig, args.out)


def _cmd_signal(args: argparse.Namespace) -> None:
    from teslasynth import render, plot
    try:
        synth = _load_synth(args.config)
        print(f"Synthesising {args.midi} …", file=sys.stderr)
        rec = render.from_file(args.midi, synth=synth, channel=args.channel)
    except FileNotFoundError as exc:
        _die(str(exc))

    start_ms = args.start_ms if args.start_ms is not None else 0.0
    end_ms   = args.end_ms   if args.end_ms   is not None else start_ms + 10.0
    fig = plot.plot_signal(rec, start_ms=start_ms, end_ms=end_ms)
    _save_or_show(fig, args.out)


def _cmd_config(args: argparse.Namespace) -> None:
    from teslasynth import config as tscfg, Configuration
    if args.config:
        try:
            cfg = tscfg.load(args.config)
        except (OSError, ValueError) as exc:
            _die(str(exc))
    else:
        cfg = Configuration()
    for expr in args.set:
        try:
            cfg.set(expr)
        except ValueError as exc:
            _die(str(exc))
    print(tscfg.dumps(cfg))


def _cmd_instruments(args: argparse.Namespace) -> None:
    from teslasynth import get_all_instruments
    all_info = get_all_instruments()
    name_w = max(len(i.name) for i in all_info)
    for info in all_info:
        env = info.envelope
        if env.type == "const":
            detail = f"level={env.sustain:.2f}"
        elif env.type == "adsr":
            detail = (f"A={env.attack_ms:.0f}ms  D={env.decay_ms:.0f}ms"
                      f"  S={env.sustain:.2f}  R={env.release_ms:.0f}ms"
                      f"  curve={env.curve}")
        else:  # ad
            detail = (f"A={env.attack_ms:.0f}ms  D={env.decay_ms:.0f}ms"
                      f"  curve={env.curve}")
        print(f"  {info.index:2d}  {info.name:{name_w}}  {env.type:5}  {detail}")


def _cmd_percussions(args: argparse.Namespace) -> None:
    from teslasynth import get_all_percussions
    all_info = get_all_percussions()
    name_w = max(len(i.name) for i in all_info)
    for info in all_info:
        env = info.envelope
        prf = f"{info.prf_hz:.0f}Hz" if info.prf_hz > 0 else "noise"
        env_detail = (f"A={env.attack_ms:.0f}ms  D={env.decay_ms:.0f}ms"
                      f"  curve={env.curve}")
        print(f"  {info.index:2d}  {info.name:{name_w}}"
              f"  burst={info.burst_ms:.0f}ms"
              f"  prf={prf:8}"
              f"  noise={info.noise*100:.0f}%"
              f"  {env_detail}")


def _cmd_envelope(args: argparse.Namespace) -> None:
    from teslasynth import plot
    preset_id = _find_instrument_or_percussion(args.instrument)
    fig = plot.plot_envelope(preset_id, note_duration_ms=args.duration_ms)
    _save_or_show(fig, args.out)


def _add_config_arg(p: argparse.ArgumentParser) -> None:
    p.add_argument("--config", metavar="FILE.json",
                   help="Load synth configuration from a JSON file "
                        "(see 'teslasynth config' for the format)")


def _add_channel_arg(p: argparse.ArgumentParser) -> None:
    p.add_argument("--channel", type=int, default=0, metavar="N",
                   help="Output channel index to use (0–7, default: 0)")


def main() -> None:
    parser = argparse.ArgumentParser(
        prog="teslasynth",
        description="Teslasynth offline synthesis and analysis tools",
    )
    sub = parser.add_subparsers(dest="command", required=True)

    # ── render ────────────────────────────────────────────────────────────────
    r = sub.add_parser("render", help="Render a MIDI file to WAV")
    r.add_argument("midi", help="Input .mid file")
    r.add_argument("wav",  help="Output .wav file")
    _add_config_arg(r)
    r.add_argument("--sample-rate", type=int, default=192_000, metavar="HZ",
                   help="Sample rate in Hz (default: 192000)")
    r.add_argument("--step-us", type=int, default=10_000, metavar="US",
                   help="Synthesis window size in µs (default: 10000)")
    _add_channel_arg(r)

    # ── plot ──────────────────────────────────────────────────────────────────
    p = sub.add_parser("plot", help="Full overview: piano roll + signal + frequency + duty")
    p.add_argument("midi", help="Input .mid file")
    _add_config_arg(p)
    p.add_argument("--out", metavar="FILE.html",
                   help="Save to HTML instead of opening the browser")
    p.add_argument("--start-ms", type=float, default=None, metavar="MS")
    p.add_argument("--end-ms",   type=float, default=None, metavar="MS")
    _add_channel_arg(p)

    # ── signal ────────────────────────────────────────────────────────────────
    sg = sub.add_parser("signal", help="Zoomed coil signal view")
    sg.add_argument("midi", help="Input .mid file")
    _add_config_arg(sg)
    sg.add_argument("--out", metavar="FILE.html",
                    help="Save to HTML instead of opening the browser")
    sg.add_argument("--start-ms", type=float, default=None, metavar="MS",
                    help="Window start in ms (default: 0)")
    sg.add_argument("--end-ms",   type=float, default=None, metavar="MS",
                    help="Window end in ms (default: start + 10 ms)")
    _add_channel_arg(sg)

    # ── config ────────────────────────────────────────────────────────────────
    c = sub.add_parser("config",
                       help="Print configuration as JSON, optionally applying key=value overrides")
    c.add_argument("--config", metavar="FILE.json",
                   help="Start from an existing config file instead of defaults")
    c.add_argument("set", nargs="*", metavar="key=value",
                   help="Apply firmware-style settings, e.g. 'synth.tuning=440hz' "
                        "'output.1.max-duty=5' 'routing.percussion=y'")

    # ── version ───────────────────────────────────────────────────────────────
    sub.add_parser("version", help="Print engine version and build info")

    # ── instruments ───────────────────────────────────────────────────────────
    sub.add_parser("instruments", help="List all built-in instruments")

    # ── percussions ───────────────────────────────────────────────────────────
    sub.add_parser("percussions", help="List all built-in percussion presets")

    # ── envelope ──────────────────────────────────────────────────────────────
    e = sub.add_parser("envelope", help="Plot envelope for an instrument")
    e.add_argument("instrument",
                   help="Instrument name or index (see 'instruments'), "
                        "or percussion name (see 'percussions')")
    e.add_argument("--out", metavar="FILE.html",
                   help="Save to HTML instead of opening the browser")
    e.add_argument("--duration-ms", type=float, default=2000.0, metavar="MS",
                   help="Note hold duration before release (default: 2000 ms)")

    args = parser.parse_args()

    if args.command == "version":
        _cmd_version(args)
    elif args.command == "render":
        _cmd_render(args)
    elif args.command == "plot":
        _cmd_plot(args)
    elif args.command == "signal":
        _cmd_signal(args)
    elif args.command == "config":
        _cmd_config(args)
    elif args.command == "instruments":
        _cmd_instruments(args)
    elif args.command == "percussions":
        _cmd_percussions(args)
    elif args.command == "envelope":
        _cmd_envelope(args)


if __name__ == "__main__":
    main()
