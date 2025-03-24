#pragma once
#include <Ty/Base.h>

typedef struct VSTEffect VSTEffect;

typedef enum VSTPluginEvent {
    VSTPluginEvent_Create,                      // No arguments.
    VSTPluginEvent_Destroy,                     // No arguments.
    VSTPluginEvent_SetPresetNumber,             // value: new program number.
    VSTPluginEvent_GetPresetNumber,             // Return current program number.
    VSTPluginEvent_SetPresetName,               // ptr: char[kVstMaxProgNameLen].
    VSTPluginEvent_GetPresetName,               // ptr: char[kVstMaxProgNameLen].

    VSTPluginEvent_GetParameterLabel,           // ptr: char[kVstMaxParamStrLen].
    VSTPluginEvent_GetParameterDisplay,         // ptr: char[kVstMaxParamStrLen].
    VSTPluginEvent_GetParameterName,            // ptr: char[kVstMaxParamStrLen].

    VSTPluginEvent__GetVu,                      // Deprecated in VST 2.4.

    VSTPluginEvent_SetSampleRate,               // opt: new sample rate for audio processing.
    VSTPluginEvent_SetBlockSize,                // value: new maximum block size for audio processing.
    VSTPluginEvent_MainsChanged,                // value: 0 means "turn off", 1 means "turn on".

    VSTPluginEvent_GetEditorRectangle,          // ptr: #ERect** receiving pointer to editor size.
    VSTPluginEvent_CreateEditor,                // ptr: system dependent Window pointer, e.g. HWND on Windows.
    VSTPluginEvent_DestroyEditor,               // No arguments.

    VSTPluginEvent__EditDraw,                   // Deprecated in VST 2.4.
    VSTPluginEvent__EditMouse,                  // Deprecated in VST 2.4.
    VSTPluginEvent__EditKey,                    // Deprecated in VST 2.4.

    VSTPluginEvent_EditorIdle,                  // No arguments.

    VSTPluginEvent__EditTop,                    // Deprecated in VST 2.4.
    VSTPluginEvent__EditSleep,                  // Deprecated in VST 2.4.
    VSTPluginEvent__Identify,                   // Deprecated in VST 2.4.

    VSTPluginEvent_GetChunk,                    // ptr: void** for chunk data address.
                                                // index: 0 for bank, 1 for program.
    VSTPluginEvent_SetChunk,                    // ptr: chunk data.
                                                // value: chunk data size.
                                                // index: 0 for bank, 1 for program.

    VSTPluginEvent_ProcessEvents,               // ptr: VstEvents*.

    VSTPluginEvent_CanBeAutomated,              // index: parameter index.
                                                // return: 1=true, 0=false.
    VSTPluginEvent_StringToParameter,           // index: parameter index.
                                                // ptr:   parameter string.
                                                // return value: 1 for success.

    VSTPluginEvent__GetNumProgramCategories,    // Deprecated in VST 2.4.

    VSTPluginEvent_GetPresetNameIndexed,        // index: program index.
                                                // ptr: char[kVstMaxProgNameLen].
                                                // return value: 1 for success.

    VSTPluginEvent__CopyProgram,                // Deprecated in VST 2.4.
    VSTPluginEvent__ConnectInput,               // Deprecated in VST 2.4.
    VSTPluginEvent__ConnectOutput,              // Deprecated in VST 2.4.

    VSTPluginEvent_GetInputProperties,          // index: input index.
                                                // ptr: VstPinProperties*.
                                                // return value: 1 if supported.
    VSTPluginEvent_GetOutputProperties,         // index: output index.
                                                // ptr: VstPinProperties*.
                                                // return value: 1 if supported.
    VSTPluginEvent_GetPlugCategory,             // return value: PluginCategory.

    VSTPluginEvent__GetCurrentPosition,         // Deprecated in VST 2.4.
    VSTPluginEvent__GetDestinationBuffer,       // Deprecated in VST 2.4.

    VSTPluginEvent_OfflineNotify,               // ptr: VstAudioFile*.
                                                // value: amount of files.
                                                // index: start flag.
    VSTPluginEvent_OfflinePrepare,              // ptr: VstOfflineTask*.
                                                // value: amount of tasks.  
    VSTPluginEvent_OfflineRun,                  // ptr: VstOfflineTask*.
                                                // value: amount of tasks.

    VSTPluginEvent_ProcessVarIo,                // ptr: VstVariableIo*.  
    VSTPluginEvent_SetSpeakerArrangement,       // value: input VstSpeakerArrangement*.
                                                // ptr:   output VstSpeakerArrangement*.

    VSTPluginEvent__SetBlockSizeAndSampleRate,  // Deprecated in VST 2.4.

    VSTPluginEvent_SetBypass,                   // value: 1 = bypass, 0 = no bypass.
    VSTPluginEvent_GetPluginName,               // ptr: char[kVstMaxEffectNameLen].

    VSTPluginEvent__GetErrorText,               // Deprecated in VST 2.4.

    VSTPluginEvent_GetAuthorName,               // ptr: char[kVstMaxVendorStrLen].
    VSTPluginEvent_GetProductName,              // ptr: char[kVstMaxProductStrLen].
    VSTPluginEvent_GetProductVersion,           // return value: vendor-specific version.
    VSTPluginEvent_VendorSpecific,              // No definition, vendor specific handling.
    VSTPluginEvent_CanDo,                       // ptr: "can do" string
                                                // return value: 0: "don't know" -1: "no" 1: "yes"  
    VSTPluginEvent_GetTailSize,                 // return value: tail size (for example the reverb time of a reverb plug-in)
                                                // 0 is default (return 1 for 'no tail')

    VSTPluginEvent__Idle,                      // Deprecated in VST 2.4.
    VSTPluginEvent__GetIcon,                   // Deprecated in VST 2.4.
    VSTPluginEvent__SetViewPosition,           // Deprecated in VST 2.4.

    VSTPluginEvent_GetParameterProperties,     // index: parameter index.
                                               // ptr: VstParameterProperties*.
                                               // return value: 1 if supported.

    VSTPluginEvent__KeysRequired,              // Deprecated in VST 2.4.

    VSTPluginEvent_GetVstVersion,              // return value: VST version.
    VSTPluginEvent_NotifyKeyDown,              // index: ASCII character.
                                               // value: virtual key-.
                                               // opt: modifiers.
                                               // return value: 1 if key used.
    VSTPluginEvent_NotifyKeyUp,                // index: ASCII character.
                                               // value: virtual key.
                                               // opt: modifiers.
                                               // return value: 1 if key used.
    VSTPluginEvent_SetEditKnobMode,            // value: knob mode.
                                               //  * 0: circular,
                                               //  * 1: circular relative,
                                               //  * 2: linear (CKnobMode in VSTGUI).

    VSTPluginEvent_GetMidiProgramName,         // index: MIDI channel.
                                               // ptr: MidiProgramName*.
                                               // return value: number of used programs, 0 if unsupported.
    VSTPluginEvent_GetCurrentMidiProgram,      // index: MIDI channel.
                                               // ptr: MidiProgramName*.
                                               // return value: index of current program.
    VSTPluginEvent_GetMidiProgramCategory,     // index: MIDI channel.
                                               // ptr: MidiProgramCategory*.
                                               // return value: number of used categories, 0 if unsupported.
    VSTPluginEvent_HasMidiProgramsChanged,     // index: MIDI channel.
                                               // return value: 1 if the MidiProgramName or MidiKeyName have changed.
    VSTPluginEvent_GetMidiKeyName,             // index: MIDI channel.
                                               // ptr: MidiKeyName*.
                                               // return value: true if supported, false otherwise.

    VSTPluginEvent_BeginSetProgram,            // No arguments.
    VSTPluginEvent_EndSetProgram,              // No arguments.

    VSTPluginEvent_GetSpeakerArrangement,      // value: input #VstSpeakerArrangement*.
                                               // ptr: output #VstSpeakerArrangement*.
    VSTPluginEvent_ShellGetNextPlugin,         // ptr: plug-in name char[kVstMaxProductStrLen].
                                               // return value: next plugin's unique_id.

    VSTPluginEvent_StartProcess,               // No arguments.
    VSTPluginEvent_StopProcess,                // No arguments.
    VSTPluginEvent_SetTotalSampleToProcess,    // value: number of samples to process,
                                               //        offline only.
    VSTPluginEvent_SetPanLaw,                  // value: pan law
                                               // opt: gain  

    VSTPluginEvent_BeginLoadBank,              // ptr: VstPatchChunkInfo*.
                                               // return value:
                                               //  * -1: bank can't be loaded,
                                               //  *  1: bank can be loaded,
                                               //  *  0: unsupported.
    VSTPluginEvent_BeginLoadProgram,           // ptr: VstPatchChunkInfo*.
                                               // return value:
                                               //  * -1: program can't be loaded,
                                               //  *  1: program can be loaded,
                                               //  *  0: unsupported  

    VSTPluginEvent_SetProcessPrecision,        // value: VstProcessPrecision.
    VSTPluginEvent_GetNumberOfMidiInputChannels,    // return value: number of used MIDI input channels (1-15).
    VSTPluginEvent_GetNumberOfMidiOutputChannels,   // return value: number of used MIDI output channels (1-15).
    VSTPluginEvent_SetTempo = 65536,            // Used in FL Studio.
                                                // value: time per beat in samples?
                                                // opt: tempo

} VSTPluginEvent;

typedef iptr(*VSTPluginDispatchFunc)(VSTEffect*, VSTPluginEvent, i32 index, iptr value, void* ptr, f32 opt);

// bool vst_plugin_create(VSTEffect*);
// bool vst_plugin_destroy(VSTEffect*);
// bool vst_plugin_set_preset_number(VSTEffect*, i32);
// iptr vst_plugin_preset_number(VSTEffect*);
// bool vst_plugin_set_preset_name(VSTEffect*, char const*);
// bool vst_plugin_preset_name(VSTEffect*, char*);
// bool vst_plugin_parameter_label()
// VSTPluginEvent_GetParameterLabel,           // ptr: char[kVstMaxParamStrLen].
// 
// VSTPluginEvent_GetParameterLabel,           // ptr: char[kVstMaxParamStrLen].
// VSTPluginEvent_GetParameterDisplay,         // ptr: char[kVstMaxParamStrLen].
// VSTPluginEvent_GetParameterName,            // ptr: char[kVstMaxParamStrLen].
// 
// VSTPluginEvent_SetSampleRate,               // opt: new sample rate for audio processing.
// VSTPluginEvent_SetBlockSize,                // value: new maximum block size for audio processing.
// VSTPluginEvent_MainsChanged,                // value: 0 means "turn off", 1 means "turn on".
// 
// VSTPluginEvent_GetEditorRectangle,          // ptr: #ERect** receiving pointer to editor size.
// VSTPluginEvent_CreateEditor,                // ptr: system dependent Window pointer, e.g. HWND on Windows.
// VSTPluginEvent_DestroyEditor,               // No arguments.
// 
// VSTPluginEvent__EditDraw,                   // Deprecated in VST 2.4.
// VSTPluginEvent__EditMouse,                  // Deprecated in VST 2.4.
// VSTPluginEvent__EditKey,                    // Deprecated in VST 2.4.
// 
// VSTPluginEvent_EditorIdle,                  // No arguments.
// 
// VSTPluginEvent__EditTop,                    // Deprecated in VST 2.4.
// VSTPluginEvent__EditSleep,                  // Deprecated in VST 2.4.
// VSTPluginEvent__Identify,                   // Deprecated in VST 2.4.
// 
// VSTPluginEvent_GetChunk,                    // ptr: void** for chunk data address.
//                                             // index: 0 for bank, 1 for program.
// VSTPluginEvent_SetChunk,                    // ptr: chunk data.
//                                             // value: chunk data size.
//                                             // index: 0 for bank, 1 for program.
// 
// VSTPluginEvent_ProcessEvents,               // ptr: VstEvents*.
// 
// VSTPluginEvent_CanBeAutomated,              // index: parameter index.
//                                             // return: 1=true, 0=false.
// VSTPluginEvent_StringToParameter,           // index: parameter index.
//                                             // ptr:   parameter string.
//                                             // return value: 1 for success.
// 
// VSTPluginEvent__GetNumProgramCategories,    // Deprecated in VST 2.4.
// 
// VSTPluginEvent_GetPresetNameIndexed,        // index: program index.
//                                             // ptr: char[kVstMaxProgNameLen].
//                                             // return value: 1 for success.
// 
// VSTPluginEvent__CopyProgram,                // Deprecated in VST 2.4.
// VSTPluginEvent__ConnectInput,               // Deprecated in VST 2.4.
// VSTPluginEvent__ConnectOutput,              // Deprecated in VST 2.4.
// 
// VSTPluginEvent_GetInputProperties,          // index: input index.
//                                             // ptr: VstPinProperties*.
//                                             // return value: 1 if supported.
// VSTPluginEvent_GetOutputProperties,         // index: output index.
//                                             // ptr: VstPinProperties*.
//                                             // return value: 1 if supported.
// VSTPluginEvent_GetPlugCategory,             // return value: PluginCategory.
// 
// VSTPluginEvent__GetCurrentPosition,         // Deprecated in VST 2.4.
// VSTPluginEvent__GetDestinationBuffer,       // Deprecated in VST 2.4.
// 
// VSTPluginEvent_OfflineNotify,               // ptr: VstAudioFile*.
//                                             // value: amount of files.
//                                             // index: start flag.
// VSTPluginEvent_OfflinePrepare,              // ptr: VstOfflineTask*.
//                                             // value: amount of tasks.  
// VSTPluginEvent_OfflineRun,                  // ptr: VstOfflineTask*.
//                                             // value: amount of tasks.
// 
// VSTPluginEvent_ProcessVarIo,                // ptr: VstVariableIo*.  
// VSTPluginEvent_SetSpeakerArrangement,       // value: input VstSpeakerArrangement*.
//                                             // ptr:   output VstSpeakerArrangement*.
// 
// VSTPluginEvent__SetBlockSizeAndSampleRate,  // Deprecated in VST 2.4.
// 
// VSTPluginEvent_SetBypass,                   // value: 1 = bypass, 0 = no bypass.
// VSTPluginEvent_GetPluginName,               // ptr: char[kVstMaxEffectNameLen].
// 
// VSTPluginEvent__GetErrorText,               // Deprecated in VST 2.4.
// 
// VSTPluginEvent_GetAuthorName,               // ptr: char[kVstMaxVendorStrLen].
// VSTPluginEvent_GetProductName,              // ptr: char[kVstMaxProductStrLen].
// VSTPluginEvent_GetProductVersion,           // return value: vendor-specific version.
// VSTPluginEvent_VendorSpecific,              // No definition, vendor specific handling.
// VSTPluginEvent_CanDo,                       // ptr: "can do" string
//                                             // return value: 0: "don't know" -1: "no" 1: "yes"  
// VSTPluginEvent_GetTailSize,                 // return value: tail size (for example the reverb time of a reverb plug-in)
//                                             // 0 is default (return 1 for 'no tail')
// 
// VSTPluginEvent__Idle,                      // Deprecated in VST 2.4.
// VSTPluginEvent__GetIcon,                   // Deprecated in VST 2.4.
// VSTPluginEvent__SetViewPosition,           // Deprecated in VST 2.4.
// 
// VSTPluginEvent_GetParameterProperties,     // index: parameter index.
//                                            // ptr: VstParameterProperties*.
//                                            // return value: 1 if supported.
// 
// VSTPluginEvent__KeysRequired,              // Deprecated in VST 2.4.
// 
// VSTPluginEvent_GetVstVersion,              // return value: VST version.
// VSTPluginEvent_NotifyKeyDown,              // index: ASCII character.
//                                            // value: virtual key-.
//                                            // opt: modifiers.
//                                            // return value: 1 if key used.
// VSTPluginEvent_NotifyKeyUp,                // index: ASCII character.
//                                            // value: virtual key.
//                                            // opt: modifiers.
//                                            // return value: 1 if key used.
// VSTPluginEvent_SetEditKnobMode,            // value: knob mode.
//                                            //  * 0: circular,
//                                            //  * 1: circular relative,
//                                            //  * 2: linear (CKnobMode in VSTGUI).
// 
// VSTPluginEvent_GetMidiProgramName,         // index: MIDI channel.
//                                            // ptr: MidiProgramName*.
//                                            // return value: number of used programs, 0 if unsupported.
// VSTPluginEvent_GetCurrentMidiProgram,      // index: MIDI channel.
//                                            // ptr: MidiProgramName*.
//                                            // return value: index of current program.
// VSTPluginEvent_GetMidiProgramCategory,     // index: MIDI channel.
//                                            // ptr: MidiProgramCategory*.
//                                            // return value: number of used categories, 0 if unsupported.
// VSTPluginEvent_HasMidiProgramsChanged,     // index: MIDI channel.
//                                            // return value: 1 if the MidiProgramName or MidiKeyName have changed.
// VSTPluginEvent_GetMidiKeyName,             // index: MIDI channel.
//                                            // ptr: MidiKeyName*.
//                                            // return value: true if supported, false otherwise.
// 
// VSTPluginEvent_BeginSetProgram,            // No arguments.
// VSTPluginEvent_EndSetProgram,              // No arguments.
// 
// VSTPluginEvent_GetSpeakerArrangement,      // value: input #VstSpeakerArrangement*.
//                                            // ptr: output #VstSpeakerArrangement*.
// VSTPluginEvent_ShellGetNextPlugin,         // ptr: plug-in name char[kVstMaxProductStrLen].
//                                            // return value: next plugin's unique_id.
// 
// VSTPluginEvent_StartProcess,               // No arguments.
// VSTPluginEvent_StopProcess,                // No arguments.
// VSTPluginEvent_SetTotalSampleToProcess,    // value: number of samples to process,
//                                            //        offline only.
// VSTPluginEvent_SetPanLaw,                  // value: pan law
//                                            // opt: gain  
// 
// VSTPluginEvent_BeginLoadBank,              // ptr: VstPatchChunkInfo*.
//                                            // return value:
//                                            //  * -1: bank can't be loaded,
//                                            //  *  1: bank can be loaded,
//                                            //  *  0: unsupported.
// VSTPluginEvent_BeginLoadProgram,           // ptr: VstPatchChunkInfo*.
//                                            // return value:
//                                            //  * -1: program can't be loaded,
//                                            //  *  1: program can be loaded,
//                                            //  *  0: unsupported  
// 
// VSTPluginEvent_SetProcessPrecision,        // value: VstProcessPrecision.
// VSTPluginEvent_GetNumberOfMidiInputChannels,    // return value: number of used MIDI input channels (1-15).
// VSTPluginEvent_GetNumberOfMidiOutputChannels,   // return value: number of used MIDI output channels (1-15).
// VSTPluginEvent_SetTempo = 65536,            // Used in FL Studio.
