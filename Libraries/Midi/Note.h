#pragma once
#include <JR/Types.h>

namespace Midi {

// clang-format off
enum class Note : u8 {
    C0, CS0, D0, DS0, E0, F0, FS0, G0, GS0, A0, AS0, B0,
    C1, CS1, D1, DS1, E1, F1, FS1, G1, GS1, A1, AS1, B1,
    C2, CS2, D2, DS2, E2, F2, FS2, G2, GS2, A2, AS2, B2,
    C3, CS3, D3, DS3, E3, F3, FS3, G3, GS3, A3, AS3, B3,
    C4, CS4, D4, DS4, E4, F4, FS4, G4, GS4, A4, AS4, B4,
    C5, CS5, D5, DS5, E5, F5, FS5, G5, GS5, A5, AS5, B5,
    C6, CS6, D6, DS6, E6, F6, FS6, G6, GS6, A6, AS6, B6,
    C7, CS7, D7, DS7, E7, F7, FS7, G7, GS7, A7, AS7, B7,
    C8, CS8, D8, DS8, E8, F8, FS8, G8, GS8, A8, AS8, B8,
    C9, CS9, D9, DS9, E9, F9, FS9, G9, GS9, A9, AS9, B9,
    C10, CS10, D10, DS10, E10, F10, FS10, G10,
    __Size
};
// clang-format on

constexpr char const* note_string(Note note)
{
    char const* note_strings[(u8)Note::__Size] {
        "C0", "CS0", "D0", "DS0", "E0", "F0", "FS0", "G0", "GS0", "A0", "AS0", "B0",
        "C1", "CS1", "D1", "DS1", "E1", "F1", "FS1", "G1", "GS1", "A1", "AS1", "B1",
        "C2", "CS2", "D2", "DS2", "E2", "F2", "FS2", "G2", "GS2", "A2", "AS2", "B2",
        "C3", "CS3", "D3", "DS3", "E3", "F3", "FS3", "G3", "GS3", "A3", "AS3", "B3",
        "C4", "CS4", "D4", "DS4", "E4", "F4", "FS4", "G4", "GS4", "A4", "AS4", "B4",
        "C5", "CS5", "D5", "DS5", "E5", "F5", "FS5", "G5", "GS5", "A5", "AS5", "B5",
        "C6", "CS6", "D6", "DS6", "E6", "F6", "FS6", "G6", "GS6", "A6", "AS6", "B6",
        "C7", "CS7", "D7", "DS7", "E7", "F7", "FS7", "G7", "GS7", "A7", "AS7", "B7",
        "C8", "CS8", "D8", "DS8", "E8", "F8", "FS8", "G8", "GS8", "A8", "AS8", "B8",
        "C9", "CS9", "D9", "DS9", "E9", "F9", "FS9", "G9", "GS9", "A9", "AS9", "B9",
        "C10", "CS10", "D10", "DS10", "E10", "F10", "FS10", "G10",
    };
    if ((u8)note >= (u8)Note::__Size)
        __builtin_abort();
    return note_strings[(u8)note];
}

constexpr f32 note_frequency(Note note)
{
    // Each note relates to each other by the equation 2^(1/12)
    using enum Note;
    f32 note_frequencies[(u8)Note::__Size] {
        [(u8)C0]   = 16.411058187037543, [(u8)CS0]  = 17.386910488549244,
        [(u8)D0]   = 18.420789987546470, [(u8)DS0]  = 19.516147160748710,
        [(u8)E0]   = 20.676637660898200, [(u8)F0]   = 21.906134517161150,
        [(u8)FS0]  = 23.208741061002520, [(u8)G0]   = 24.588804620673365,
        [(u8)GS0]  = 26.050931030014740, [(u8)A0]   = 27.500000000000000,
        [(u8)AS0]  = 29.241181404316553, [(u8)B0]   = 30.979952533338700,
        [(u8)C1]   = 32.703195662574814, [(u8)CS1]  = 34.647828872108995,
        [(u8)D1]   = 36.708095989675930, [(u8)DS1]  = 38.890872965260100,
        [(u8)E1]   = 41.203444614108730, [(u8)F1]   = 43.653528929125480,
        [(u8)FS1]  = 46.249302838954290, [(u8)G1]   = 48.999429497718660,
        [(u8)GS1]  = 51.913087197493140, [(u8)A1]   = 55.000000000000000,
        [(u8)AS1]  = 58.270470189761240, [(u8)B1]   = 61.735412657015516,
        [(u8)C2]   = 65.406391325149630, [(u8)CS2]  = 69.295657744217990,
        [(u8)D2]   = 73.416191979351860, [(u8)DS2]  = 77.781745930520200,
        [(u8)E2]   = 82.406889228217470, [(u8)F2]   = 87.307057858250960,
        [(u8)FS2]  = 92.498605677908590, [(u8)G2]   = 97.998858995437320,
        [(u8)GS2]  = 103.82617439498628, [(u8)A2]   = 110.00000000000000,
        [(u8)AS2]  = 116.54094037952248, [(u8)B2]   = 123.47082531403103,
        [(u8)C3]   = 130.81278265029925, [(u8)CS3]  = 138.59131548843598,
        [(u8)D3]   = 146.83238395870373, [(u8)DS3]  = 155.56349186104040,
        [(u8)E3]   = 164.81377845643493, [(u8)F3]   = 174.61411571650190,
        [(u8)FS3]  = 184.99721135581717, [(u8)G3]   = 195.99771799087463,
        [(u8)GS3]  = 207.65234878997256, [(u8)A3]   = 220.00000000000000,
        [(u8)AS3]  = 233.08188075904496, [(u8)B3]   = 246.94165062806206,
        [(u8)C4]   = 261.62556530059850, [(u8)CS4]  = 277.18263097687196,
        [(u8)D4]   = 293.66476791740746, [(u8)DS4]  = 311.12698372208080,
        [(u8)E4]   = 329.62755691286986, [(u8)F4]   = 349.22823143300380,
        [(u8)FS4]  = 369.99442271163434, [(u8)G4]   = 391.99543598174927,
        [(u8)GS4]  = 415.30469757994510, [(u8)A4]   = 440.00000000000000,
        [(u8)AS4]  = 466.16376151808990, [(u8)B4]   = 493.88330125612410,
        [(u8)C5]   = 523.25113060119700, [(u8)CS5]  = 554.36526195374390,
        [(u8)D5]   = 587.32953583481490, [(u8)DS5]  = 622.25396744416160,
        [(u8)E5]   = 659.25511382573970, [(u8)F5]   = 698.45646286600770,
        [(u8)FS5]  = 739.98884542326870, [(u8)G5]   = 783.99087196349850,
        [(u8)GS5]  = 830.60939515989030, [(u8)A5]   = 880.00000000000000,
        [(u8)AS5]  = 932.32752303617990, [(u8)B5]   = 987.76660251224830,
        [(u8)C6]   = 1046.5022612023940, [(u8)CS6]  = 1108.7305239074879,
        [(u8)D6]   = 1174.6590716696298, [(u8)DS6]  = 1244.5079348883232,
        [(u8)E6]   = 1318.5102276514795, [(u8)F6]   = 1396.9129257320153,
        [(u8)FS6]  = 1479.9776908465374, [(u8)G6]   = 1567.9817439269970,
        [(u8)GS6]  = 1661.2187903197805, [(u8)A6]   = 1760.0000000000000,
        [(u8)AS6]  = 1864.6550460723597, [(u8)B6]   = 1975.5332050244965,
        [(u8)C7]   = 2093.0045224047880, [(u8)CS7]  = 2217.4610478149757,
        [(u8)D7]   = 2349.3181433392597, [(u8)DS7]  = 2489.0158697766465,
        [(u8)E7]   = 2637.0204553029590, [(u8)F7]   = 2793.8258514640306,
        [(u8)FS7]  = 2959.9553816930747, [(u8)G7]   = 3135.9634878539940,
        [(u8)GS7]  = 3322.4375806395610, [(u8)A7]   = 3520.0000000000000,
        [(u8)AS7]  = 3729.3100921447194, [(u8)B7]   = 3951.0664100489930,
        [(u8)C8]   = 4186.0090448095760, [(u8)CS8]  = 4434.9220956299510,
        [(u8)D8]   = 4698.6362866785190, [(u8)DS8]  = 4978.0317395532930,
        [(u8)E8]   = 5274.0409106059180, [(u8)F8]   = 5587.6517029280610,
        [(u8)FS8]  = 5919.9107633861495, [(u8)G8]   = 6271.9269757079880,
        [(u8)GS8]  = 6644.8751612791220, [(u8)A8]   = 7040.0000000000000,
        [(u8)AS8]  = 7458.6201842894390, [(u8)B8]   = 7902.1328200979860,
        [(u8)C9]   = 8372.0180896191520, [(u8)CS9]  = 8869.8441912599030,
        [(u8)D9]   = 9397.2725733570390, [(u8)DS9]  = 9956.0634791065860,
        [(u8)E9]   = 10548.081821211836, [(u8)F9]   = 11175.303405856122,
        [(u8)FS9]  = 11839.821526772299, [(u8)G9]   = 12543.853951415977,
        [(u8)GS9]  = 13289.750322558244, [(u8)A9]   = 14080.000000000000,
        [(u8)AS9]  = 14917.240368578878, [(u8)B9]   = 15804.265640195972,
        [(u8)C10]  = 16744.036179238305, [(u8)CS10] = 17739.688382519806,
        [(u8)D10]  = 18794.545146714077, [(u8)DS10] = 19912.126958213170,
        [(u8)E10]  = 21096.163642423670, [(u8)F10]  = 22350.606811712245,
        [(u8)FS10] = 23679.643053544598, [(u8)G10]  = 25087.707902831953,
    };
    if ((u8)note >= (u8)Note::__Size)
        __builtin_abort();
    return note_frequencies[(u8)note];
}


}
