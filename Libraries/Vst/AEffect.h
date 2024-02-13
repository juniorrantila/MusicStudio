#pragma once
#include <Ty/Base.h>

namespace Vst {

constexpr auto MAX_NAME_LENGTH = 64;
constexpr auto MAX_LONG_LABEL_LENGTH = 64;
constexpr auto MAX_SHORT_LABEL_LENGTH = 8;
constexpr auto MAX_CATEGORY_LABEL_LENGTH = 24;
constexpr auto MAX_FILENAME_LENGTH = 100;

using ParameterFlagsUnderlying = i32;
enum class ParameterFlags : ParameterFlagsUnderlying {
    IsSwitch                = 1 << 0,
    UsesIntegerMinMax       = 1 << 1,
    UsesFloatStep           = 1 << 2,
    UsesIntStep             = 1 << 3,
    SupportsDisplayIndex    = 1 << 4,
    SupportsDisplayCategory = 1 << 5,
    CanRampUpOrDown         = 1 << 6,
};

struct ParameterProperties
{
    f32 step_float { 0.0 };
    f32 small_step_float { 0.0 };
    f32 large_step_float { 0.0 };
    char label[MAX_LONG_LABEL_LENGTH] {};
    ParameterFlagsUnderlying flags { 0 };
    i32 min_integer { 0 };
    i32 max_integer { 0 };
    i32 step_integer { 0 };
    i32 large_step_integer { 0 };
    char short_label[MAX_SHORT_LABEL_LENGTH] {};

    i16 display_index { 0 }; // index where this parameter
                             // should be displayed.

    i16 category { 0 }; // 0: no category, else group index + 1
    i16 parameters_in_category { 0 };
    i16 reserved { 0 };
    char category_label[MAX_CATEGORY_LABEL_LENGTH] { 0 };

    u8 future[16] {};
};

enum class EventType : i32 {
    Midi = 1,
    _Audio,     // Deprecated.
    _Video,     // Deprecated. 
    _Parameter, // Deprecated.
    _Trigger,   // Deprecated. 
    MidiSystemExclusive
};


struct Event
{
    EventType type;
    i32 struct_size_excluding_type_field_and_this;
    i32 delta_frames;
    i32 flags;
    u8 data[];

    // clang-format off
    u32 full_size() const
    {
        return sizeof(EventType) +
               sizeof(struct_size_excluding_type_field_and_this) +
               struct_size_excluding_type_field_and_this;
    }
    // clang-format on
};

struct Events 
{
    i32 number_of_events;
    iptr reserved { 0 };
    Event* events[];
};

using MidiSystemExclusiveEventFlagsUnderlying = i32;
enum class MidiSystemExclusiveEventFlags : MidiSystemExclusiveEventFlagsUnderlying { };
struct MidiSystemExclusiveEvent
{
    EventType type;
    i32 struct_size_excluding_type_field_and_this;
    i32 delta_frames;
    MidiSystemExclusiveEventFlagsUnderlying flags;

    i32 system_exclusive_dump_size;
    iptr reserved1 { 0 };
    u8* system_exclusive_dump;
    iptr reserved2 { 0 };
};

using MidiEventFlagsUnderlying = i32;
enum class MidiEventFlags : MidiEventFlagsUnderlying {
    IsRealtime = 1 << 0
};
struct MidiEvent
{
    EventType type;
    i32 struct_size_excluding_type_field_and_this;
    i32 delta_frames;
    MidiEventFlagsUnderlying flags;

    i32 note_length_in_samples;
    i32 note_offset_in_samples;
    u8 midi_data[4];
    i8 detune_cents;
    i8 note_off_velocity;
    u8 reserved1 { 0 };
    u8 reserved2 { 0 };
};

using TimeInfoFlagsUnderlying = i32;
enum class TimeInfoFlags : TimeInfoFlagsUnderlying {
    TransportDidChange          = 1 << 0,
    TransportIsPlaying          = 1 << 1,
    TransportCycleMode          = 1 << 2,
    TransportRecordingMode      = 1 << 3,
    AutomationIsWriting         = 1 << 6,
    AutomationIsReading         = 1 << 7,
    NanosecondsIsValid          = 1 << 8,
    MusicalPositionValid        = 1 << 9,
    TempoIsValid                = 1 << 10,
    BarStartPositionisValid     = 1 << 11,
    CyclePosIsValid             = 1 << 12,
    TimeSignatureIsValid        = 1 << 13,
    SmpteIsValid                = 1 << 14,
    SamplesToNextClockIsValid   = 1 << 15
};

enum class SmpteFrameRate : i32
{
    Fps24           = 0,
    Fps25           = 1,
    Fps29Point97    = 2,
    Fps30           = 3,
    Drop29Point97   = 4,
    Drop30          = 5,
    Film16mm        = 6,
    Film35mm        = 7,
    Fps23Point976   = 10,
    Fps24Point976   = 11,
    Fps59Point94    = 12,
    Fps60           = 13
};

struct TimeInfo
{
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
    SmpteFrameRate smpte_framerate;
    i32 samples_to_next_clock_24_per_quarter_note;
    TimeInfoFlagsUnderlying flags;
};

struct VariableIO
{
    float** inputs;
    float** outputs;
    i32 number_of_input_samples;
    i32 number_of_output_samples;
    i32* number_of_input_samples_processed;
    i32* number_of_output_samples_processed;
};

enum class HostLanguage : i32 {
    Unknown,
    English,
    German,
    French,
    Italian,
    Spanish,
    Japanese
};

using PinPropertiesFlagsUnderlying = i32;
enum class PinPropertiesFlags : ParameterFlagsUnderlying
{
    IsActive   = 1 << 0,
    IsStereo   = 1 << 1,
    UseSpeaker = 1 << 2
};

struct PinProperties
{
    char label[MAX_LONG_LABEL_LENGTH];
    PinPropertiesFlagsUnderlying flags;
    i32 arrangement_type;
    char short_label[MAX_SHORT_LABEL_LENGTH];
    u8 future[48];
};

enum class PluginCategory : i32
{
    Unknown,
    Effect,
    Synth,
    Analysis,
    Mastering,
    Spacializer,
    RoomFx,
    SurroundFx,
    Restoration,
    OfflineProcess,
    Shell,
    Generator,
    __Size
};

enum class MidiProgramNameFlags : i32
{
    IsOmni = 1
};

struct MidiPresetName
{
    i32 this_program_index;
    char name[MAX_NAME_LENGTH];
    i8 midi_program;
    i8 midi_bank_msb;
    i8 midi_bank_lsb;
    u8 reserved;
    i32 parent_category_index;
    MidiProgramNameFlags flags;
};

struct MidiProgramCategory
{
    i32 this_category_index;
    char name[MAX_NAME_LENGTH];
    i32 parent_category_index;
    i32 flags; // None defined.
};

struct MidiKeyName
{
    i32 this_program_index;
    i32 this_key_number;
    char keyName[MAX_NAME_LENGTH];
    i32 reserved;
    i32 flags; // None defined.
};

enum class SpeakerType : i32 {
    Undefined = 0x7fffffff,
    Mono = 0,
    Left,
    Right,
    Center,
    Subbass,
    LeftSurround,
    RightSurround,
    LeftOfCenter,
    RightOfCenter,
    Surround,
    CenterOfSurround = Surround,
    SideLeft,
    SideRight,
    TopMiddle,
    TopFrontLeft,
    TopfrontCenter,
    TopFrontRight,
    TopRearLeft,
    TopRearCenter,
    TopRearRight,
    Subbass2
};

enum class SpeakerArrangementType : i32 {
    UserDefined = -2,
    Empty = -1,
    Mono  =  0,         // M
    Stereo,             // L R
    StereoSurround,     // Ls Rs
    StereoCenter,       // Lc Rc
    StereoSide,         // Sl Sr
    StereoCLfe,         // C Lfe
    Cine3Point0,        // L R C
    Music3Point0,       // L R S
    Cine3Point1,        // L R C Lfe
    Music3Point1,       // L R Lfe S
    Cine4Point0,        // L R C   S (LCRS)
    Music4Point0,       // L R Ls  Rs (Quadro)
    Cine4Point1,        // L R C   Lfe S (LCRS+Lfe)
    Music4Point1,       // L R Lfe Ls Rs (Quadro+Lfe)
    FivePoint0,         // L R C Ls  Rs
    FivePoint1,         // L R C Lfe Ls Rs
    Cine6Point,         // L R C   Ls  Rs Cs
    Music6Point0,       // L R Ls  Rs  Sl Sr
    Cine6Point1,        // L R C   Lfe Ls Rs Cs
    Music6Point1,       // L R Lfe Ls  Rs Sl Sr
    Cine7Point0,        // L R C Ls  Rs Lc Rc
    Music7Point0,       // L R C Ls  Rs Sl Sr
    Cine7Point1,        // L R C Lfe Ls Rs Lc Rc
    Music7Point1,       // L R C Lfe Ls Rs Sl Sr
    Cine8Point0,        // L R C Ls  Rs Lc Rc Cs
    Music8Point0,       // L R C Ls  Rs Cs Sl Sr
    Cine8Point1,        // L R C Lfe Ls Rs Lc Rc Cs
    Music8Point1,       // L R C Lfe Ls Rs Cs Sl Sr
    TenPoint2,          // L R C Lfe Ls Rs Tfl Tfc Tfr Trl Trr Lfe2
    __Size
};

struct SpeakerProperties
{
    float azimuth;        ///< unit: rad, range: -PI...PI, exception: 10.f for LFE channel
    float elevation;    ///< unit: rad, range: -PI/2...PI/2, exception: 10.f for LFE channel
    float radius;        ///< unit: meter, exception: 0.f for LFE channel
    float reserved;        ///< zero (reserved for future use)
    char name[MAX_NAME_LENGTH];    ///< for new setups, new names should be given (L/R/C... won't do)
    SpeakerType type;        ///< @see VstSpeakerType

    u8 future[28];    ///< reserved for future use
};

struct SpeakerArrangement
{
    SpeakerArrangementType type;                        ///< e.g. #kSpeakerArr51 for 5.1  @see VstSpeakerArrangementType
    i32 numChannels;                ///< number of channels in this speaker arrangement
    SpeakerProperties speakers[8];    ///< variable sized speaker array
};

enum class UserSpeakerType : i32 {
    U32 = -32,
    U31,
    U30,
    U29,
    U28,
    U27,
    U26,
    U25,
    U24,
    U23,
    U22,
    U21,
    U20, ///< == #kSpeakerLfe2
    U19, ///< == #kSpeakerTrr
    U18, ///< == #kSpeakerTrc
    U17, ///< == #kSpeakerTrl
    U16, ///< == #kSpeakerTfr
    U15, ///< == #kSpeakerTfc
    U14, ///< == #kSpeakerTfl
    U13, ///< == #kSpeakerTm
    U12, ///< == #kSpeakerSr
    U11, ///< == #kSpeakerSl
    U10, ///< == #kSpeakerCs
    U9,  ///< == #kSpeakerS
    U8,  ///< == #kSpeakerRc
    U7,  ///< == #kSpeakerLc
    U6,  ///< == #kSpeakerRs
    U5,  ///< == #kSpeakerLs
    U4,  ///< == #kSpeakerLfe
    U3,  ///< == #kSpeakerC
    U2,  ///< == #kSpeakerR
    U1   ///< == #kSpeakerL
};

using OfflineTaskFlagsUnderlying = i32;
enum class OfflineTaskFlags : OfflineTaskFlagsUnderlying {
    UnvalidParameter    = 1 << 0,   // set by host.
    NewFile             = 1 << 1,   // set by Host
    PlugError           = 1 << 10,  // set by plug-in
    InterleavedAudio    = 1 << 11,  // set by plug-in
    TempOutputFile      = 1 << 12,  // set by plug-in
    FloatOutputFile     = 1 << 13,  // set by plug-in
    RandomWrite         = 1 << 14,  // set by plug-in
    Stretch             = 1 << 15,  // set by plug-in
    NoThread            = 1 << 16   // set by plug-in
};


struct OfflineTask
{
    char process_name[96];              // Set by plugin.

    f64 read_position;                  // Set by host or plugin.
    f64 write_position;                 // Set by host or plugin.
    i32 read_count;                     // Set by host or plugin.
    i32 write_count;                    // Set by plugin.
    i32 input_buffer_size;              // Set by host.
    i32 output_buffer_size;             // Set by host.
    void* input_buffer;                 // Set by host.
    void* output_buffer;                // Set by host.
    f64 position_to_process_from;       // Set by host.
    f64 frames_to_process;              // Set by host.
    f64 max_frames_to_write;            // Set by plugin.

    void* extra_buffer;                 // Set by plugin.
    i32 value;                          // Set by host or plugin.
    i32 index;                          // Set by host or plugin.

    f64 frames_in_source_file;          // Set by host.
    f64 source_sample_rate;             // Set by host or plugin.
    f64 destination_sample_rate;        // Set by host or plugin.
    i32 source_channels;                // Set by host or plugin.
    i32 destination_channels;           // Set by host or plugin.
    i32 source_format;                  // Set by host.
    i32 destination_format;             // Set by plugin.
    char output_text[512];              // Set by plugin or host.

    f64 progress;                       // Set by plugin.
    i32 progress_mode;                  // Reserved for future use.
    char progress_text[100];            // Set by plugin.

    OfflineTaskFlagsUnderlying flags;   // Set by host and plugin.
    i32 return_value;                   // Reserved for future use.
    void* host_owned;                   // Set by host.
    void* plugin_owned;                 // Set by plugin.

    u8 future[1024];                    // Reserved for future use.
};

using OfflineOptionUnderlying = i32;
enum class OfflineOption : OfflineTaskFlagsUnderlying {
    ReadWriteAudio,
    ReadPeaks,
    ReadWriteParameters,
    ReadWriteMarkers,
    ReadWriteCursor,
    ReadAndChangeSelection,
    RequestAsyncQueryFiles,
};

using AudioFileFlagsUnderlying = i32;
enum class AudioFileFlags : AudioFileFlagsUnderlying
{
    HostIsReadOnly            = 1 << 0,  // Set by Host in call offline_notify().
    HostHasNoRateConversion   = 1 << 1,  // Set by Host in call offline_notify().
    HostHasNoChannelChange    = 1 << 2,  // Set by Host in call offline_notify().
    PluginCanProcessSelection = 1 << 10, // Set by plugin in call offline_start().
    PluginHasNoCrossfade      = 1 << 11, // Set by plugin in call offline_start().
    PluginWantsToRead         = 1 << 12, // Set by plugin in call offline_start().
    PluginWantsToWrite        = 1 << 13, // Set by plugin in call offline_start().
    PluginWantsWriteMarker    = 1 << 14, // Set by plugin in call offline_start().
    PluginWantsMoveCursor     = 1 << 15, // Set by plugin in call offline_start().
    PluginWantsSelect         = 1 << 16, // Set by plugin in call offline_start().
};

struct AudioFile
{
    AudioFileFlagsUnderlying flags;
    void* host_owned;
    void* plugin_owned;
    char name[MAX_FILENAME_LENGTH];
    i32 unique_id;
    f64 sample_rate;
    i32 number_of_channels;
    f64 number_of_frames;
    i32 format;             // Reserved for future use.
    f64 edit_cursor_position;
    f64 selection_start;    // Frame index of first selected frame, or -1.
    f64 selection_size;
    i32 selected_channels_mask;
    i32 number_of_markers;
    i32 time_ruler_unit;    // See doc for possible values.
    f64 time_ruler_offset;  // (positive or negative)
    f64 tempo;
    i32 time_signature_numerator;
    i32 time_signature_denominator;
    i32 ticks_per_black_note;
    i32 smpte_frame_rate;

    u8 future[64];
};

struct AudioFileMarker
{
    f64 position;
    char name[32];
    i32 type;
    i32 id;
    i32 reserved;
};

struct _Window {
    char title[128];
    i16 x;
    i16 y;
    i16 width;
    i16 height;
    i32 style;
    void* parent;
    void* user_handle;
    void* window_handle;

    u8 future[104];
};

enum class VirtualKey : u8;

using ModifierKeyUnderlying = u8;
enum class ModifierKey : ModifierKeyUnderlying;

struct KeyCode
{
    i32 character;
    VirtualKey virtual_key;
    ModifierKeyUnderlying modifier;
};

enum class VirtualKey : u8 {
    Back = 1,
    Tab,
    Clear,
    Return,
    Pause,
    Escape,
    Space,
    Next,
    End,
    Home,
    Left,
    Up,
    Right,
    Down,
    PageUp,
    PageDown,
    Select,
    Print,
    Enter,
    Snapshot,
    Insert,
    Delete,
    Help,
    Numpad0,
    Numpad1,
    Numpad2,
    Numpad3,
    Numpad4,
    Numpad5,
    Numpad6,
    Numpad7,
    Numpad8,
    Numpad9,
    Multiply,
    Add,
    Separator,
    Subtract,
    Decimal,
    Divide,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    Numlock,
    Scroll,
    Shift,
    Control,
    Alt,
    Equals
};

enum class ModifierKey : ModifierKeyUnderlying {
    Shift     = 1<<0,
    Alternate = 1<<1,
    Command   = 1<<2,
    Control   = 1<<3
};

struct FileType
{
    char name[128];
    char macos_type[8];
    char dos_type[8];
    char unix_type[8];
    char mime_type1[128];
    char mime_type2[128];
};

enum class FileSelectCommand : i32 {
    Load,
    Save,
    LoadMultiple,
    SelectFolder
};

enum class FileSelectType : i32 {
    File
};

struct FileSelect
{
    FileSelectCommand command;
    FileSelectType type;
    i32 macos_creator;
    i32 file_types_size;
    FileType* file_types;
    char title[1024];
    char* initial_path;
    char* return_path;              // Use with kVstFileLoad and kVstDirectorySelect.
                                    // null: Host allocates memory, plugin must call closeOpenFileSelector!
    i32 return_path_size;           // Size of allocated memory for return paths.
    char** multiple_return_paths;   // Use with kVstMultipleFilesLoad.
                                    // Host allocates memory, plugin must call #closeOpenFileSelector!
    i32 multiple_return_paths_size;
    iptr reserved;

    u8 future[116];            ///< reserved for future use
};

struct PatchChunkInfo
{
    i32 version { 1 };            ///< Format Version (should be 1)
    i32 plugin_unique_id; 
    i32 plugin_version;
    i32 number_of_banks_or_parameters;
    u8 future[48];            ///< Reserved for future use
};

enum class PanLaw : i32 {
    Linar,      // L = pan * M; R = (1 - pan) * M;
    EqualPower  // L = pow(pan, 0.5) * M; R = pow((1 - pan), 0.5) * M;
};

enum class ProcessLevel : i32 {
    Unknown, 
    GUI,
    Realtime,
    Sequencer,
    Offline,
};

enum class AutomationState : i32 {
    Unsupported,
    Off,
    Read,
    Write,
    ReadWrite
};

}
