#pragma once
#include <Ty/Base.h>

namespace Vst {

enum class CanDo {
    No = -1,
    Maybe = 0,
    Yes = 1
};

namespace CanDoStrings {
    inline constexpr auto SendVstEvents         = "sendVstEvents";
    inline constexpr auto SendVstMidiEvent      = "sendVstMidiEvent";
    inline constexpr auto RecieveVstEvents      = "receiveVstEvents";
    inline constexpr auto RecieveVstMidiEvent   = "receiveVstMidiEvent";
    inline constexpr auto RecieveVstTimeInfo    = "receiveVstTimeInfo";
    inline constexpr auto Offline               = "offline";
    inline constexpr auto MidiProgramNames      = "midiProgramNames";
    inline constexpr auto Bypass                = "bypass";
    inline constexpr auto RecieveVstSysexEvent  = "receiveVstSysexEvent";
    inline constexpr auto ViewDpiScaling        = "supportsViewDpiScaling"; // FL Studio.
    inline constexpr auto MidiSingleNoteTuning  = "midiSingleNoteTuningChange"; // Bitwig.
    inline constexpr auto MidiKeyBasedInstrumentControl = "midiKeyBasedInstrumentControl"; // Bitwig.
    inline constexpr auto HasCockosNoScrollUI   = "hasCockosNoScrollUI"; // Reaper.
    inline constexpr auto WantsChannelCountNotifications = "wantsChannelCountNotifications"; // Reaper.
    inline constexpr auto HasCockosExtensions   = "hasCockosExtensions"; // Reaper.

}
}

