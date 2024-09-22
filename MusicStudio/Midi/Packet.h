#pragma once
#include <Ty/Base.h>
#include <Midi/Note.h>

namespace Midi {

enum class PacketType : u8 {
    NoteOff = 0x80,
    NoteOn  = 0x90,
    Aftertouch = 0xA0,
    ContinousControll = 0xB0,
    PatchChange = 0xC0,
    ChannelPressure = 0xD0,
    PitchBend = 0xE0,
    NonMusicalCommands = 0xF0,
};
struct Packet {
    struct NoteOff {
        Note note;
        u8 velocity;
    };
    constexpr Packet(NoteOff value) 
        : type(PacketType::NoteOff)
        , data{(u8)value.note, value.velocity, 0}
    {
    }

    struct NoteOn {
        Note note;
        u8 velocity;
    };
    constexpr Packet(NoteOn value) 
        : type(PacketType::NoteOn)
        , data{(u8)value.note, value.velocity, 0}
    {
    }

    struct Aftertouch {
        Note note;
        u8 touch;
    };
    constexpr Packet(Aftertouch value) 
        : type(PacketType::Aftertouch)
        , data{(u8)value.note, value.touch, 0}
    {
    }

    struct ContinousControll {
        u8 controller;
        u8 controller_value;
    };
    constexpr Packet(ContinousControll value) 
        : type(PacketType::ContinousControll)
        , data{value.controller, value.controller_value, 0}
    {
    }

    struct PatchChange {
        u8 instrument;
    };
    constexpr Packet(PatchChange value) 
        : type(PacketType::PatchChange)
        , data{value.instrument, 0, 0}
    {
    }

    struct ChannelPressure {
        u8 pressure;
    };
    constexpr Packet(ChannelPressure value) 
        : type(PacketType::ChannelPressure)
        , data{value.pressure, 0, 0}
    {
    }

    struct PitchBend {
        u8 least_significant_byte;
        u8 most_significant_byte;
    };
    constexpr Packet(PitchBend value) 
        : type(PacketType::PitchBend)
        , data{value.least_significant_byte, value.most_significant_byte, 0}
    {
    }

    struct NonMusicalCommands {
        u8 unknown0;
        u8 unknown1;
    };
    constexpr Packet(NonMusicalCommands value) 
        : type(PacketType::NonMusicalCommands)
        , data{value.unknown0, value.unknown1, 0}
    {
    }

    PacketType type;
    u8 data[3];

    constexpr Packet(u8 byte1, u8 byte2, u8 byte3)
        : type((PacketType)byte1)
        , data { byte2, byte3, 0 }
    {
    }

    constexpr u8 size() const
    {
        switch (type) {
            case PacketType::NoteOff:
                return sizeof(NoteOff);
            case PacketType::NoteOn: 
                return sizeof(NoteOn);
            case PacketType::Aftertouch: 
                return sizeof(Aftertouch);
            case PacketType::ContinousControll:
                return sizeof(ContinousControll);
            case PacketType::PatchChange:
                return sizeof(PatchChange);
            case PacketType::ChannelPressure:
                return sizeof(ChannelPressure);
            case PacketType::PitchBend:
                return sizeof(PitchBend);
            case PacketType::NonMusicalCommands:
                return sizeof(NonMusicalCommands);
        }
    }

    constexpr NoteOff as_note_off() const
    {
        return NoteOff {
            .note = (Note)data[0],
            .velocity = data[1]
        };
    }

    constexpr NoteOn as_note_on() const
    {
        return NoteOn {
            .note = (Note)data[0],
            .velocity = data[1]
        };
    }

    constexpr Aftertouch as_aftertouch() const
    {
        return {
            .note = (Note)data[0],
            .touch = data[1],
        };
    }

    constexpr ContinousControll as_continous_controll() const
    {
        return {
            .controller = data[0],
            .controller_value = data[1],
        };
    }

    constexpr PatchChange as_patch_change() const
    {
        return {
            .instrument = data[0],
        };
    }

    constexpr ChannelPressure as_channel_pressure() const
    {
        return {
            .pressure = data[0],
        };
    }

    constexpr PitchBend as_pitch_bend() const
    {
        return {
            .least_significant_byte = data[0],
            .most_significant_byte  = data[1],
        };
    }

    constexpr NonMusicalCommands as_non_musical_commands() const
    {
        return {
            .unknown0 = data[0],
            .unknown1 = data[1],
        };
    }
};


}
