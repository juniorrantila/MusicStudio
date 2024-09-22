#pragma once
#include <Ty/Base.h>
#include <Ty/StringView.h>
#include <Ty/View.h>

namespace Vst {

struct CanDo {
    enum class Kind : i32 {
        No = -1,
        Maybe = 0,
        Yes = 1
    };
    using enum Kind;

    constexpr CanDo(iptr kind)
        : m_kind((Kind)kind)
    {
    }
    explicit operator iptr() const { return (iptr)m_kind; }

    constexpr CanDo(Kind kind)
        : m_kind(kind)
    {
    }

    constexpr operator Kind() const { return m_kind; }

    constexpr StringView name() const
    {
        switch(*this) {
        case No: return "no"sv;
        case Maybe: return "maybe"sv;
        case Yes: return "yes"sv;
        }
    }

private:
    enum Kind m_kind;
};

struct Feature {
    enum class Kind {
        SendVstEvents,
        SendVstMidiEvent,
        RecieveVstEvents,
        RecieveVstMidiEvent,
        RecieveVstTimeInfo,
        Offline,
        MidiProgramNames,
        Bypass,
        RecieveVstSysexEvent,

        ViewDpiScaling,
        ViewEmbedding,
        ViewResize,

        MidiSingleNoteTuning,
        MidiKeyBasedInstrumentControl,
        HasCockosNoScrollUI,
        WantsChannelCountNotifications,
        HasCockosExtensions,
    };
    using enum Kind;

    constexpr StringView name() const
    {
        switch (m_kind) {
        case SendVstEvents: return "sendVstEvents"sv;
        case SendVstMidiEvent: return "sendVstMidiEvent"sv;
        case RecieveVstEvents: return "receiveVstEvents"sv;
        case RecieveVstMidiEvent: return "receiveVstMidiEvent"sv;
        case RecieveVstTimeInfo: return "receiveVstTimeInfo"sv;
        case Offline: return "offline"sv;
        case MidiProgramNames: return "midiProgramNames"sv;
        case Bypass: return "bypass"sv;
        case RecieveVstSysexEvent: return "receiveVstSysexEvent"sv;

        case ViewDpiScaling: return "supportsViewDpiScaling"sv; // Prosonussv;
        case ViewEmbedding: return "supportsViewEmbedding"sv;  // Prosonussv;
        case ViewResize: return "supportsViewResize"sv;     // Prosonussv;

        case MidiSingleNoteTuning: return "midiSingleNoteTuningChange"sv; // Bitwigsv;
        case MidiKeyBasedInstrumentControl: return "midiKeyBasedInstrumentControl"sv; // Bitwigsv;
        case HasCockosNoScrollUI: return "hasCockosNoScrollUI"sv; // Reapersv;
        case WantsChannelCountNotifications: return "wantsChannelCountNotifications"sv; // Reapersv;
        case HasCockosExtensions: return "hasCockosExtensions"sv; // Reapersv;
        }
    }

    constexpr Feature(Kind kind)
        : m_kind(kind)
    {
    }

    static Optional<Feature> parse(StringView name)
    {
        for (auto feature : all()) {
            if (name == feature.name())
                return feature;
        }
        return {};
    }

    static View<Feature const> all()
    {
        static constexpr Feature features[] = {
            SendVstEvents,
            SendVstMidiEvent,
            RecieveVstEvents,
            RecieveVstMidiEvent,
            RecieveVstTimeInfo,
            Offline,
            MidiProgramNames,
            Bypass,
            RecieveVstSysexEvent,

            // Prosonus
            ViewDpiScaling,
            ViewEmbedding,
            ViewResize,

            // Bitwig
            MidiSingleNoteTuning,
            MidiKeyBasedInstrumentControl,

            // Reaper
            HasCockosNoScrollUI,
            WantsChannelCountNotifications,
            HasCockosExtensions,
        };
        return View(features, sizeof(features) / sizeof(features[0]));
    }

    constexpr operator Kind() const { return m_kind; }
    constexpr operator void*() const { return (void*)name().data(); }
    constexpr operator c_string() const { return name().data(); }

private:
    enum Kind m_kind;
};

}
