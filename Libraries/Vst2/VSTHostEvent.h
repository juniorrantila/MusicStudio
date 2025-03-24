#pragma once
#include "./Forward.h"

#include <Ty/Base.h>
#if __APPLE__
#include <Carbon/Carbon.h>
#endif

#include "./Language.h"

typedef enum VSTHostEvent {
    VSTHostEvent_Automate,                       // index: parameter index.
                                                 // opt: parameter value.
    VSTHostEvent_VstVersion,                     // return value: Host VST version (2400 for VST 2.4).
    VSTHostEvent_CurrentId,                      // return value: current unique identifier on shell plugin.
    VSTHostEvent_Idle,                           // No arguments.
    VSTHostEvent__PinConnected,                  // Deprecated in VST 2.4 r2.

    VSTHostEvent__WantMidi = VSTHostEvent__PinConnected + 2,
    VSTHostEvent_GetTime,                        // return value: TimeInfo* or null if not supported.
                                                 // value: request mask.
    VSTHostEvent_ProcessEvents,                  // ptr: VstEvents*.

    VSTHostEvent__SetTime,                       // Deprecated in VST 2.4.
    VSTHostEvent__TempoAt,                       // Deprecated in VST 2.4.
    VSTHostEvent__GetNumAutomatableParameters,   // Deprecated in VST 2.4.
    VSTHostEvent__GetParameterQuantization,      // Deprecated in VST 2.4.

    VSTHostEvent_IOChanged,                      // return value: 1 if supported.

    VSTHostEvent__NeedIdle,                      // Deprecated in VST 2.4.

    VSTHostEvent_ResizeWindow,                   // index: new width.
                                                 // value: new height.
                                                 // return value: 1 if supported.
    VSTHostEvent_GetSampleRate,                  // return value: current sample rate.
    VSTHostEvent_GetBlockSize,                   // return value: current block size.
    VSTHostEvent_GetInputLatency,                // return value: input latency in audio samples.
    VSTHostEvent_GetOutputLatency,               // return value: output latency in audio samples.

    VSTHostEvent__GetPreviousPlug,               // Deprecated in VST 2.4.
    VSTHostEvent__GetNextPlug,                   // Deprecated in VST 2.4.
    VSTHostEvent__WillReplaceOrAccumulate,       // Deprecated in VST 2.4.

    VSTHostEvent_GetCurrentProcessLevel,         // return value: current process level.
    VSTHostEvent_GetAutomationState,             // return value: current automation state.

    VSTHostEvent_OfflineStart,                   // index: new audio files size.
                                                 // value: audio files size.
                                                 // ptr: AudioFile*.
    VSTHostEvent_OfflineRead,                    // index: bool read_source.
                                                 // value: OfflineOption*.
                                                 // ptr: OfflineTask*.
    VSTHostEvent_OfflineWrite,
    VSTHostEvent_OfflineGetCurrentPass,
    VSTHostEvent_OfflineGetCurrentMetaPass,

    VSTHostEvent__SetOutputSampleRate,           // Deprecated in VST 2.4.
    VSTHostEvent__GetOutputSpeakerArrangement,   // Deprecated in VST 2.4.

    VSTHostEvent_GetAuthorName,                  // ptr: char[kVstMaxVendorStrLen]
    VSTHostEvent_GetHostName,                    // ptr: char[kVstMaxProductStrLen]
    VSTHostEvent_GetVendorVersion,               // return value: vendor specific version.
    VSTHostEvent_VendorSpecific,                 // no definition, vendor specific handling.

    VSTHostEvent__SetIcon,                       // Deprecated in VST 2.4.

    VSTHostEvent_CanDo,                          // ptr: "can do" string.
                                                 // return value: 1 for supported.
    VSTHostEvent_GetLanguage,                    // return value language code.

    VSTHostEvent__OpenWindow,                    // Deprecated in VST 2.4.
    VSTHostEvent__CloseWindow,                   // Deprecated in VST 2.4.

    VSTHostEvent_GetDirectory,                   // return value: FSSpec on MAC, else char*.
    VSTHostEvent_UpdateDisplay,                  // no arguments.
    VSTHostEvent_BeginEdit,                      // index: parameter index.
    VSTHostEvent_EndEdit,                        // index: parameter index.
    VSTHostEvent_OpenFileSelector,               // ptr: FileSelect*.
                                                 // return value: 1 if supported.
    VSTHostEvent_CloseFileSelector,              // ptr: FileSelect*.

    VSTHostEvent__EditFile,                      // Deprecated in VST 2.4.

    VSTHostEvent__GetChunkFile,                  // Deprecated in VST 2.4
                                                 // ptr: char[2048] or char[sizeof(FSSpec)]
                                                 // return value: 1 if supported 

    VSTHostEvent__GetInputSpeakerArrangement     // Deprecated in VST 2.4.
} VSTHostEvent;

typedef iptr(*VSTHostDispatchFunc)(VSTEffect*, VSTHostEvent, i32 index, iptr value, void* ptr, f32 opt);
typedef struct VSTHostDispatcher {
    VSTHostDispatchFunc dispatch;
    VSTEffect* effect;
} VSTHostDispatcher;

C_API inline VSTHostDispatcher vst_make_host_dispatcher(VSTHostDispatchFunc dispatch, VSTEffect* effect)
{
    return (VSTHostDispatcher){
        .dispatch = dispatch,
        .effect = effect,
    };
}

C_API bool vst_host_automate(VSTHostDispatcher dispatcher, i32 parameter_index, f32 value);
C_API iptr vst_host_vst_version(VSTHostDispatcher dispatcher);
C_API iptr vst_host_current_id(VSTHostDispatcher dispatcher);
C_API bool vst_host_idle(VSTHostDispatcher dispatcher);
C_API VSTTimeInfo* vst_host_time(VSTHostDispatcher);
C_API bool vst_host_process_events(VSTHostDispatcher dispatcher, VSTEvents* events);
C_API bool vst_host_io_changed(VSTHostDispatcher dispatcher);
C_API bool vst_host_resize_window(VSTHostDispatcher dispatcher, i32 width, i32 height);
C_API iptr vst_host_sample_rate(VSTHostDispatcher dispatcher);
C_API iptr vst_host_block_size(VSTHostDispatcher dispatcher);
C_API iptr vst_host_input_latency(VSTHostDispatcher dispatcher);
C_API iptr vst_host_output_latency(VSTHostDispatcher dispatcher);
C_API iptr vst_host_current_process_level(VSTHostDispatcher dispatcher);
C_API iptr vst_host_automation_state(VSTHostDispatcher dispatcher);
C_API iptr vst_host_author_name(VSTHostDispatcher dispatcher, char*);
C_API iptr vst_host_host_name(VSTHostDispatcher dispatcher, char*);
C_API iptr vst_host_vendor_version(VSTHostDispatcher dispatcher);
C_API iptr vst_host_vendor_specific(VSTHostDispatcher dispatcher, i32, iptr, void*, f32);
C_API iptr vst_host_can_do(VSTHostDispatcher dispatcher, char const*);
VSTLanguage vst_host_language(VSTHostDispatcher dispatcher);
#if __APPLE__
C_API FSSpec* vst_host_directory(VSTHostDispatcher dispatcher);
#else
C_API char* vst_host_directory(VSTHostDispatcher dispatcher);
#endif
C_API bool vst_host_update_display(VSTHostDispatcher dispatcher);
C_API bool vst_host_begin_edit(VSTHostDispatcher dispatcher, i32 parameter);
C_API bool vst_host_end_edit(VSTHostDispatcher dispatcher, i32 parameter);
C_API bool vst_host_open_file_selector(VSTHostDispatcher dispatcher, VSTFileSelect*);
C_API bool vst_host_close_file_selector(VSTHostDispatcher dispatcher, VSTFileSelect*);
