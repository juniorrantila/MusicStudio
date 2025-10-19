#pragma once
#include <Ty/Base.h>

typedef enum VSTSmpteFrameRate {
    VSTSmpteFrameRate_Fps24            = 0,
    VSTSmpteFrameRate_Fps25            = 1,
    VSTSmpteFrameRate_Fps29Point97     = 2,
    VSTSmpteFrameRate_Fps30            = 3,
    VSTSmpteFrameRate_Drop29Point97    = 4,
    VSTSmpteFrameRate_Drop30           = 5,
    VSTSmpteFrameRate_Film16mm         = 6,
    VSTSmpteFrameRate_Film35mm         = 7,
    VSTSmpteFrameRate_Fps23Point976    = 10,
    VSTSmpteFrameRate_Fps24Point976    = 11,
    VSTSmpteFrameRate_Fps59Point94     = 12,
    VSTSmpteFrameRate_Fps60            = 13,
} VSTSmpteFrameRate;

typedef enum VSTTimeInfoFlags {
    VSTTimeInfoFlags_TransportDidChange        = 1 << 0,
    VSTTimeInfoFlags_TransportIsPlaying        = 1 << 1,
    VSTTimeInfoFlags_TransportCycleMode        = 1 << 2,
    VSTTimeInfoFlags_TransportRecordingMode    = 1 << 3,
    VSTTimeInfoFlags_AutomationIsWriting       = 1 << 6,
    VSTTimeInfoFlags_AutomationIsReading       = 1 << 7,
    VSTTimeInfoFlags_NanosecondsIsValid        = 1 << 8,
    VSTTimeInfoFlags_MusicalPositionValid      = 1 << 9,
    VSTTimeInfoFlags_TempoIsValid              = 1 << 10,
    VSTTimeInfoFlags_BarStartPositionisValid   = 1 << 11,
    VSTTimeInfoFlags_CyclePosIsValid           = 1 << 12,
    VSTTimeInfoFlags_TimeSignatureIsValid      = 1 << 13,
    VSTTimeInfoFlags_SmpteIsValid              = 1 << 14,
    VSTTimeInfoFlags_SamplesToNextClockIsValid = 1 << 15
} VSTTimeInfoFlags;

typedef struct VSTTimeInfo {
    f64 sample_position;
    f64 sample_rate_in_hertz;
    f64 system_time_in_nanoseconds;
    f64 musical_position_in_quarter_notes;
    f64 tempo_in_bpm;
    f64 last_bar_start_position_in_quarter_notes;
    f64 cycle_start_position_in_quarter_notes;
    f64 cycle_end_position_in_quarter_notes;
    i32 time_signature_numerator;
    i32 time_signature_denominator;
    i32 smpte_offset;
    VSTSmpteFrameRate smpte_framerate;
    i32 samples_to_next_clock_24_per_quarter_note;
    i32 flags; // VSTTimeInfoFlags
} VSTTimeInfo;
