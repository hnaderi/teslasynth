#include <nanobind/nanobind.h>
#include <nanobind/stl/array.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

#include "teslasynth/midi_synth.hpp"
#include "teslasynth/config_patch_update.hpp"
#include "synthesizer/envelope.hpp"
#include "synthesizer/instruments.hpp"
#include "synthesizer/bank/percussions.hpp"

namespace nb = nanobind;
using namespace nb::literals;

using namespace teslasynth::midisynth;
using namespace teslasynth::midi;
using namespace teslasynth::synth;
using namespace teslasynth::core;
using namespace teslasynth::synth::envelopes;
using namespace teslasynth::synth::bank;
using namespace teslasynth::midisynth::config;

// The Python library always uses 8 outputs — no MCU memory constraints here.
using Synth   = Teslasynth<8>;
using Config  = Configuration<8>;
using Routing = MidiRoutingConfig<8>;

// 200 pulses covers up to ~20 kHz at the default 10 ms step.
// written[] is uint8_t so the hard ceiling is 255.
using Buffer = PulseBuffer<8, 200>;

// Flat Python-visible envelope, avoiding std::variant exposure
struct PyEnvelope {
    std::string type;      // "adsr", "ad", "const"
    float       attack_ms;
    float       decay_ms;
    float       sustain;
    float       release_ms;
    std::string curve;     // "linear", "exponential"
};

static PyEnvelope to_py_envelope(const EnvelopeConfig &cfg) {
    return std::visit([](auto const &e) -> PyEnvelope {
        using T = std::decay_t<decltype(e)>;
        if constexpr (std::is_same_v<T, ADSR>) {
            return {
                "adsr",
                e.attack.micros()  / 1000.0f,
                e.decay.micros()   / 1000.0f,
                static_cast<float>(e.sustain),
                e.release.micros() / 1000.0f,
                e.type == CurveType::Lin ? "linear" : "exponential",
            };
        } else if constexpr (std::is_same_v<T, AD>) {
            return {
                "ad",
                e.attack.micros() / 1000.0f,
                e.decay.micros()  / 1000.0f,
                0.0f,
                0.0f,
                e.type == CurveType::Lin ? "linear" : "exponential",
            };
        } else {
            return {"const", 0.0f, 0.0f, static_cast<float>(e), 0.0f, "linear"};
        }
    }, cfg);
}

static nb::dict percussion_to_dict(PercussionId id) {
    const size_t idx = static_cast<size_t>(id);
    const Percussion &p = percussion_kit[idx];
    nb::dict d;
    d["index"]    = static_cast<int>(idx);
    d["id"]       = id;
    d["name"]     = std::string(percussion_names[idx]);
    d["burst_ms"] = p.burst.micros() / 1000.0f;
    d["prf_hz"]   = static_cast<float>(p.prf);
    d["noise"]    = static_cast<float>(p.noise);
    d["skip"]     = static_cast<float>(p.skip);
    d["envelope"] = to_py_envelope(p.envelope);
    return d;
}

NB_MODULE(_teslasynth, m) {
    m.doc() = "Teslasynth C++ synthesis engine bindings";

    m.def("version", []() -> std::string { return TESLASYNTH_VERSION; },
          "Return the engine version string (git describe at build time).");
    m.def("build_info", []() -> nb::dict {
        nb::dict d;
        d["version"] = std::string(TESLASYNTH_VERSION);
        d["date"]    = std::string(TESLASYNTH_BUILD_DATE);
        d["time"]    = std::string(TESLASYNTH_BUILD_TIME);
        return d;
    }, "Return a dict with version, date and time of the build.");

    // -------------------------------------------------------------------------
    // Enums
    // -------------------------------------------------------------------------

    nb::enum_<MidiMessageType>(m, "MidiMessageType")
        .value("NoteOff",           MidiMessageType::NoteOff)
        .value("NoteOn",            MidiMessageType::NoteOn)
        .value("AfterTouchPoly",    MidiMessageType::AfterTouchPoly)
        .value("ControlChange",     MidiMessageType::ControlChange)
        .value("ProgramChange",     MidiMessageType::ProgramChange)
        .value("AfterTouchChannel", MidiMessageType::AfterTouchChannel)
        .value("PitchBend",         MidiMessageType::PitchBend);

    nb::enum_<InstrumentId>(m, "InstrumentId")
        .value("SquareWave",       InstrumentId::SquareWave)
        .value("MonoLead",         InstrumentId::MonoLead)
        .value("SoftLead",         InstrumentId::SoftLead)
        .value("BrightLead",       InstrumentId::BrightLead)
        .value("SyncLead",         InstrumentId::SyncLead)
        .value("SawLead",          InstrumentId::SawLead)
        .value("SynthPluck",       InstrumentId::SynthPluck)
        .value("HarpPluck",        InstrumentId::HarpPluck)
        .value("EPluck",           InstrumentId::EPluck)
        .value("FMKey",            InstrumentId::FMKey)
        .value("BellKey",          InstrumentId::BellKey)
        .value("SubBass",          InstrumentId::SubBass)
        .value("AnalogBass",       InstrumentId::AnalogBass)
        .value("RubberBass",       InstrumentId::RubberBass)
        .value("AcidBass",         InstrumentId::AcidBass)
        .value("WarmPad",          InstrumentId::WarmPad)
        .value("SlowPad",          InstrumentId::SlowPad)
        .value("ChoirPad",         InstrumentId::ChoirPad)
        .value("GlassPad",         InstrumentId::GlassPad)
        .value("MotionPad",        InstrumentId::MotionPad)
        .value("Organ",            InstrumentId::Organ)
        .value("Brass",            InstrumentId::Brass)
        .value("SoftBrass",        InstrumentId::SoftBrass)
        .value("Strings",          InstrumentId::Strings)
        .value("StaccatoStrings",  InstrumentId::StaccatoStrings)
        .value("SynthHit",         InstrumentId::SynthHit)
        .value("NoiseHit",         InstrumentId::NoiseHit)
        .value("RiseFX",           InstrumentId::RiseFX)
        .value("FallFX",           InstrumentId::FallFX);

    nb::enum_<PercussionId>(m, "PercussionId")
        .value("Kick",       PercussionId::Kick)
        .value("Snare",      PercussionId::Snare)
        .value("Clap",       PercussionId::Clap)
        .value("ClosedHat",  PercussionId::ClosedHat)
        .value("OpenHat",    PercussionId::OpenHat)
        .value("LowTom",     PercussionId::LowTom)
        .value("MidTom",     PercussionId::MidTom)
        .value("HighTom",    PercussionId::HighTom)
        .value("Rimshot",    PercussionId::Rimshot)
        .value("Cowbell",    PercussionId::Cowbell)
        .value("Shaker",     PercussionId::Shaker)
        .value("Crash",      PercussionId::Crash)
        .value("Ride",       PercussionId::Ride);

    // -------------------------------------------------------------------------
    // Envelope (flattened static config)
    // -------------------------------------------------------------------------

    nb::class_<PyEnvelope>(m, "Envelope")
        .def_ro("type",       &PyEnvelope::type,       "One of 'adsr', 'ad', 'const'")
        .def_ro("attack_ms",  &PyEnvelope::attack_ms)
        .def_ro("decay_ms",   &PyEnvelope::decay_ms)
        .def_ro("sustain",    &PyEnvelope::sustain,    "Sustain level [0, 1]")
        .def_ro("release_ms", &PyEnvelope::release_ms)
        .def_ro("curve",      &PyEnvelope::curve,      "'linear' or 'exponential'")
        .def("__repr__", [](const PyEnvelope &e) {
            if (e.type == "const")
                return "Envelope(type='const', level=" + std::to_string(e.sustain) + ")";
            return "Envelope(type='" + e.type +
                   "', curve='" + e.curve +
                   "', attack=" + std::to_string(e.attack_ms) +
                   "ms, decay="   + std::to_string(e.decay_ms) +
                   "ms, sustain=" + std::to_string(e.sustain) +
                   ", release="   + std::to_string(e.release_ms) + "ms)";
        });

    // -------------------------------------------------------------------------
    // Instrument info
    // -------------------------------------------------------------------------

    m.def("get_instrument", [](InstrumentId id) -> nb::dict {
        const size_t idx = static_cast<size_t>(id);
        nb::dict d;
        d["index"]            = static_cast<int>(idx);
        d["id"]               = id;
        d["name"]             = std::string(instrument_names[idx]);
        d["envelope"]         = to_py_envelope(instruments[idx].envelope);
        d["vibrato_rate_hz"]  = static_cast<float>(instruments[idx].vibrato.freq);
        d["vibrato_depth_hz"] = static_cast<float>(instruments[idx].vibrato.depth);
        return d;
    }, "id"_a, "Return a dict describing one instrument.");

    m.def("get_all_instruments", []() -> nb::list {
        nb::list result;
        for (size_t i = 0; i < instruments_size; i++) {
            nb::dict d;
            d["index"]            = static_cast<int>(i);
            d["id"]               = static_cast<InstrumentId>(i);
            d["name"]             = std::string(instrument_names[i]);
            d["envelope"]         = to_py_envelope(instruments[i].envelope);
            d["vibrato_rate_hz"]  = static_cast<float>(instruments[i].vibrato.freq);
            d["vibrato_depth_hz"] = static_cast<float>(instruments[i].vibrato.depth);
            result.append(d);
        }
        return result;
    }, "Return a list of dicts for all built-in instruments.");

    m.def("get_percussion", [](PercussionId id) -> nb::dict {
        return percussion_to_dict(id);
    }, "id"_a, "Return a dict describing one percussion preset.");

    m.def("get_all_percussions", []() -> nb::list {
        nb::list result;
        for (size_t i = 0; i < percussion_size; i++)
            result.append(percussion_to_dict(static_cast<PercussionId>(i)));
        return result;
    }, "Return a list of dicts for all built-in percussion presets.");

    m.def("percussion_for_note", [](uint8_t note) -> nb::dict {
        return percussion_to_dict(midi_to_percussion(note));
    }, "note"_a,
       "Return the percussion preset dict for a given MIDI drum note (0–127).");

    // -------------------------------------------------------------------------
    // MidiChannelMessage
    // -------------------------------------------------------------------------

    nb::class_<MidiChannelMessage>(m, "MidiChannelMessage")
        // No default constructor — there is no valid zero-initialised message.
        .def_rw("type", &MidiChannelMessage::type)
        .def_prop_rw("channel",
            [](const MidiChannelMessage &m) { return static_cast<uint8_t>(m.channel); },
            [](MidiChannelMessage &m, uint8_t v) { m.channel = v; })
        .def_prop_rw("data0",
            [](const MidiChannelMessage &m) { return static_cast<uint8_t>(m.data0); },
            [](MidiChannelMessage &m, uint8_t v) { m.data0 = v; })
        .def_prop_rw("data1",
            [](const MidiChannelMessage &m) { return static_cast<uint8_t>(m.data1); },
            [](MidiChannelMessage &m, uint8_t v) { m.data1 = v; })
        .def_static("note_on",  [](uint8_t ch, uint8_t note, uint8_t vel) {
            return MidiChannelMessage::note_on(ch, note, vel);
        }, "ch"_a, "note"_a, "vel"_a = 127)
        .def_static("note_off", [](uint8_t ch, uint8_t note, uint8_t vel) {
            return MidiChannelMessage::note_off(ch, note, vel);
        }, "ch"_a, "note"_a, "vel"_a = 0)
        .def_static("program_change", [](uint8_t ch, uint8_t value) {
            return MidiChannelMessage::program_change(ch, value);
        })
        .def_static("pitchbend", [](uint8_t ch, uint16_t value) {
            return MidiChannelMessage::pitchbend(ch, value);
        })
        .def_static("control_change", [](uint8_t ch, uint8_t cc, uint8_t value) {
            return MidiChannelMessage::control_change(
                ch, static_cast<ControlChange>(cc), value);
        })
        .def("__repr__", [](const MidiChannelMessage &m) {
            return std::string(m);
        });

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    nb::class_<ChannelConfig>(m, "ChannelConfig")
        .def(nb::init<>())
        .def_prop_rw("max_on_time_us",
            [](const ChannelConfig &c) { return c.max_on_time.micros(); },
            [](ChannelConfig &c, uint16_t v) { c.max_on_time = Duration16::micros(v); },
            "Maximum pulse-on duration in microseconds")
        .def_prop_rw("min_deadtime_us",
            [](const ChannelConfig &c) { return c.min_deadtime.micros(); },
            [](ChannelConfig &c, uint16_t v) { c.min_deadtime = Duration16::micros(v); },
            "Minimum pulse-off duration in microseconds")
        .def_prop_rw("max_duty_percent",
            [](const ChannelConfig &c) { return c.max_duty.percent(); },
            [](ChannelConfig &c, float v) { c.max_duty = DutyCycle(v); },
            "Maximum duty cycle [0, 100]")
        .def_prop_rw("duty_window_us",
            [](const ChannelConfig &c) { return c.duty_window.micros(); },
            [](ChannelConfig &c, uint16_t v) { c.duty_window = Duration16::micros(v); },
            "Duty-cycle averaging window in microseconds (default 10000 = 10 ms)")
        .def_rw("notes", &ChannelConfig::notes, "Polyphony limit (1–4)")
        .def_prop_rw("instrument",
            [](const ChannelConfig &c) -> std::optional<uint8_t> { return c.instrument; },
            [](ChannelConfig &c, std::optional<uint8_t> v) { c.instrument = v; },
            "Channel-specific instrument override (None = use global)")
        .def("__repr__", [](const ChannelConfig &c) { return std::string(c); });

    // -------------------------------------------------------------------------
    // RoutingConfig — must be bound before Configuration references it
    // -------------------------------------------------------------------------

    nb::class_<Routing>(m, "RoutingConfig")
        .def(nb::init<>())
        .def_prop_rw("mapping",
            [](const Routing &r) -> nb::list {
                nb::list result;
                for (const auto &opt : r.mapping) {
                    auto v = opt.value();
                    if (v.has_value())
                        result.append(static_cast<int>(static_cast<uint8_t>(*v)));
                    else
                        result.append(nb::none());
                }
                return result;
            },
            [](Routing &r, nb::list lst) {
                if (lst.size() != 16)
                    throw nb::value_error(
                        "mapping must have exactly 16 entries (one per MIDI channel 0–15)");
                size_t i = 0;
                for (auto &slot : r.mapping) {
                    nb::object item = lst[i++];
                    if (item.is_none()) {
                        slot = OutputNumberOpt<8>();
                    } else {
                        int v = nb::cast<int>(item);
                        if (v < 0 || v >= 8)
                            throw nb::value_error(
                                "output index must be in [0, 7]");
                        slot = OutputNumberOpt<8>(static_cast<int8_t>(v));
                    }
                }
            },
            "16-entry list mapping MIDI channels 0–15 to an output index (0–7) or None.")
        .def_rw("percussion", &Routing::percussion,
            "When True, MIDI channel 9 is treated as a percussion channel.")
        .def("__repr__", [](const Routing &r) {
            std::string s = "[";
            size_t i = 0;
            for (const auto &opt : r.mapping) {
                if (i++) s += ", ";
                auto v = opt.value();
                s += v.has_value() ? std::to_string(static_cast<uint8_t>(*v)) : "None";
            }
            s += "]";
            return "RoutingConfig(percussion=" +
                   std::string(r.percussion ? "True" : "False") +
                   ", mapping=" + s + ")";
        });

    nb::class_<SynthConfig>(m, "SynthConfig")
        .def(nb::init<>())
        .def_prop_rw("tuning_hz",
            [](const SynthConfig &s) { return static_cast<float>(s.tuning); },
            [](SynthConfig &s, float v) { s.tuning = Hertz(v); },
            "Concert tuning (default 440 Hz)")
        .def_prop_rw("instrument",
            [](const SynthConfig &s) -> std::optional<uint8_t> { return s.instrument; },
            [](SynthConfig &s, std::optional<uint8_t> v) { s.instrument = v; },
            "Global default instrument (None = use per-channel selection)")
        .def("__repr__", [](const SynthConfig &s) { return std::string(s); });

    nb::class_<Config>(m, "Configuration")
        .def(nb::init<>())
        .def_prop_rw("synth",
            [](Config &c) -> SynthConfig & { return c.synth(); },
            [](Config &c, const SynthConfig &s) { c.synth() = s; },
            nb::rv_policy::reference_internal)
        .def_prop_ro("channels_size", &Config::channels_size,
            "Number of output channels (8).")
        .def("channel", [](Config &c, uint8_t ch) -> ChannelConfig & {
            if (ch >= c.channels_size())
                throw nb::index_error(
                    ("channel index " + std::to_string(ch) +
                     " out of range (0–7)").c_str());
            return c.channel(ch);
        }, nb::rv_policy::reference_internal, "ch"_a,
           "Return the ChannelConfig for output channel *ch* (0–7).")
        .def_prop_rw("routing",
            [](Config &c) -> Routing & { return c.routing(); },
            [](Config &c, const Routing &r) { c.routing() = r; },
            nb::rv_policy::reference_internal,
            "MIDI-to-output routing configuration.")
        .def("__repr__", [](const Config &c) { return std::string(c); })
        .def("set",
            [](Config &cfg, std::string expr) {
                auto res = config::patch::update(std::string_view(expr), cfg);
                if (!res)
                    throw nb::value_error(res.error().c_str());
            },
            "expr"_a,
            "Apply a key=value expression using the firmware CLI syntax.\n"
            "Examples: 'synth.tuning=440hz', 'output.1.max-duty=5', "
            "'routing.percussion=y'");

    // -------------------------------------------------------------------------
    // Pulse — a single coil drive pulse returned by sample_all()
    // -------------------------------------------------------------------------

    nb::class_<Pulse>(m, "Pulse")
        .def("__init__", [](Pulse *self, uint16_t on_us, uint16_t off_us) {
            new (self) Pulse{Duration16::micros(on_us), Duration16::micros(off_us)};
        }, "on_us"_a, "off_us"_a, "Construct a Pulse from on/off durations in microseconds.")
        .def_prop_ro("on_us",  [](const Pulse &p) -> uint16_t { return p.on.micros(); },
                     "Pulse-on duration in microseconds.")
        .def_prop_ro("off_us", [](const Pulse &p) -> uint16_t { return p.off.micros(); },
                     "Pulse-off duration in microseconds.")
        .def("__repr__", [](const Pulse &p) {
            return "Pulse(on_us=" + std::to_string(p.on.micros()) +
                   ", off_us=" + std::to_string(p.off.micros()) + ")";
        })
        .def("__eq__", [](const Pulse &a, const Pulse &b) {
            return a.on.micros() == b.on.micros() &&
                   a.off.micros() == b.off.micros();
        });

    // -------------------------------------------------------------------------
    // EnvelopeEngine — exact C++ Envelope implementation
    // -------------------------------------------------------------------------

    nb::class_<Envelope>(m, "EnvelopeEngine")
        .def("__init__", [](Envelope *self, InstrumentId id) {
            new (self) Envelope(instruments[static_cast<size_t>(id)].envelope);
        }, "id"_a, "Create an envelope engine for the given instrument.")
        .def("__init__", [](Envelope *self, PercussionId id) {
            new (self) Envelope(percussion_kit[static_cast<size_t>(id)].envelope);
        }, "id"_a, "Create an envelope engine for the given percussion preset.")
        .def("update",
            [](Envelope &e, uint32_t delta_us, bool note_held) -> float {
                return static_cast<float>(e.update(Duration32::micros(delta_us), note_held));
            },
            "delta_us"_a, "note_held"_a,
            "Advance by delta_us µs. Pass note_held=False to begin release. "
            "Returns amplitude [0, 1].")
        .def_prop_ro("is_off", &Envelope::is_off,
            "True once the envelope has fully completed (including release).");

    // -------------------------------------------------------------------------
    // Teslasynth engine
    // -------------------------------------------------------------------------

    nb::class_<Synth>(m, "Teslasynth")
        .def(nb::init<>(), "Create a synth with default configuration (8 output channels).")
        .def(nb::init<const Config &>(), "cfg"_a,
             "Create a synth with the given configuration.")
        .def("handle",
            [](Synth &s, const MidiChannelMessage &msg, int64_t time_us) {
                if (time_us < 0)
                    throw nb::value_error("time_us must be non-negative");
                s.handle(msg, Duration::micros(static_cast<uint64_t>(time_us)));
            },
            "msg"_a, "time_us"_a,
            "Feed a MIDI message at the given absolute time (microseconds).")
        .def("sample_all",
            [](Synth &s, uint32_t budget_us) -> nb::list {
                if (budget_us > 65535)
                    throw nb::value_error(
                        "budget_us must be ≤ 65535 (Duration16 limit ~65 ms). "
                        "Use a smaller step_us.");
                Buffer buf;
                s.sample_all(Duration16::micros(static_cast<uint16_t>(budget_us)), buf);
                nb::list result;
                for (uint8_t ch = 0; ch < 8; ch++) {
                    const uint8_t n = buf.written[ch];
                    if (n >= Buffer::output_bufsize)
                        PyErr_WarnEx(PyExc_RuntimeWarning,
                            "sample_all: pulse buffer full — pulses may have been dropped. "
                            "Reduce step_us or the synthesis frequency.", 1);
                    nb::list ch_pulses;
                    for (uint8_t i = 0; i < n; i++) {
                        Pulse p = buf.at(ch, i);
                        ch_pulses.append(nb::cast(std::move(p)));
                    }
                    result.append(ch_pulses);
                }
                return result;
            },
            "budget_us"_a,
            "Synthesise up to budget_us µs (max 65535). "
            "Returns a list of 8 lists (one per output channel), "
            "each containing [on_us, off_us] pairs.")
        .def("off",           &Synth::off,
             "Silence all voices immediately.")
        .def("reload_config", &Synth::reload_config,
             "Apply configuration changes (also calls off()).")
        .def_prop_rw("configuration",
            [](Synth &s) -> Config & { return s.configuration(); },
            [](Synth &s, const Config &c) {
                s.configuration() = c;
                s.reload_config();
            });
}
