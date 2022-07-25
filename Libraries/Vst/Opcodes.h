#pragma once
#include <JR/Types.h>

namespace Vst {

enum class HostOpcode : i32 {
    Automate,                       // index: parameter index.
                                    // opt: parameter value.
    VstVersion,                        // return value: Host VST version (2400 for VST 2.4).
    CurrentId,                      // return value: current unique identifier on shell plugin.
    Idle,                           // No arguments.
    _PinConnected,                  // Deprecated in VST 2.4 r2.

    _WantMidi = _PinConnected + 2,
    GetTime,                        // return value: TimeInfo* or null if not supported.
                                    // value: request mask.
    ProcessEvents,                  // ptr: VstEvents*.

    _SetTime,                       // Deprecated in VST 2.4.
    _TempoAt,                       // Deprecated in VST 2.4.
    _GetNumAutomatableParameters,   // Deprecated in VST 2.4.
    _GetParameterQuantization,      // Deprecated in VST 2.4.

    IOChanged,                      // return value: 1 if supported.

    _NeedIdle,                      // Deprecated in VST 2.4.

    ResizeWindow,                   // index: new width.
                                    // value: new height.
                                    // return value: 1 if supported.
    GetSampleRate,                  // return value: current sample rate.
    GetBlockSize,                   // return value: current block size.
    GetInputLatency,                // return value: input latency in audio samples.
    GetOutputLatency,               // return value: output latency in audio samples.

    _GetPreviousPlug,               // Deprecated in VST 2.4.
    _GetNextPlug,                   // Deprecated in VST 2.4.
    _WillReplaceOrAccumulate,       // Deprecated in VST 2.4.

    GetCurrentProcessLevel,         // return value: current process level.
    GetAutomationState,             // return value: current automation state.

    OfflineStart,                   // index: new audio files size.
                                    // value: audio files size.
                                    // ptr: AudioFile*.
    OfflineRead,                    // index: bool read_source.
                                    // value: OfflineOption*.
                                    // ptr: OfflineTask*.
    OfflineWrite,
    OfflineGetCurrentPass,
    OfflineGetCurrentMetaPass,

    _SetOutputSampleRate,           // Deprecated in VST 2.4.
    _GetOutputSpeakerArrangement,   // Deprecated in VST 2.4.

    GetAuthorName,                  // ptr: char[kVstMaxVendorStrLen]
    GetHostName,                    // ptr: char[kVstMaxProductStrLen]
    GetVendorVersion,               // return value: vendor specific version.
    VendorSpecific,                 // no definition, vendor specific handling.

    _SetIcon,                       // Deprecated in VST 2.4.

    CanDo,                          // ptr: "can do" string.
                                    // return value: 1 for supported.
    GetLanguage,                    // return value language code.

    _OpenWindow,                    // Deprecated in VST 2.4.
    _CloseWindow,                   // Deprecated in VST 2.4.

    GetDirectory,                   // return value: FSSpec on MAC, else char*.
    UpdateDisplay,                  // no arguments.
    BeginEdit,                      // index: parameter index.
    EndEdit,                        // index: parameter index.
    OpenFileSelector,               // ptr: FileSelect*.
                                    // return value: 1 if supported.
    CloseFileSelector,              // ptr: FileSelect*.

    _EditFile,                      // Deprecated in VST 2.4.

    _GetChunkFile,                  // Deprecated in VST 2.4
                                    // ptr: char[2048] or char[sizeof(FSSpec)]
                                    // return value: 1 if supported 

    _GetInputSpeakerArrangement     // Deprecated in VST 2.4.
};

enum class PluginOpcode : i32 {
    Create,                     // No arguments.
    Destroy,                    // No arguments.
    SetPresetNumber,            // value: new program number.
    GetPresetNumber,            // Return current program number.
    SetPresetName,              // ptr: char[kVstMaxProgNameLen].
    GetPresetName,              // ptr: char[kVstMaxProgNameLen].

    GetParameterLabel,              // ptr: char[kVstMaxParamStrLen].
    GetParameterDisplay,            // ptr: char[kVstMaxParamStrLen].
    GetParameterName,               // ptr: char[kVstMaxParamStrLen].

    _GetVu,                     // Deprecated in VST 2.4.

    SetSampleRate,              // opt: new sample rate for audio processing.
    SetBlockSize,               // value: new maximum block size for audio processing.
    MainsChanged,               // value: 0 means "turn off", 1 means "turn on".

    GetEditorRectangle,                // ptr: #ERect** receiving pointer to editor size.
    CreateEditor,                   // ptr: system dependent Window pointer, e.g. HWND on Windows.
    DestroyEditor,                  // No arguments.

    _EditDraw,                  // Deprecated in VST 2.4.
    _EditMouse,                 // Deprecated in VST 2.4.
    _EditKey,                   // Deprecated in VST 2.4.

    EditorIdle,                   // No arguments.

    _EditTop,                   // Deprecated in VST 2.4.
    _EditSleep,                 // Deprecated in VST 2.4.
    _Identify,                  // Deprecated in VST 2.4.

    GetChunk,                   // ptr: void** for chunk data address.
                                // index: 0 for bank, 1 for program.
    SetChunk,                   // ptr: chunk data.
                                // value: chunk data size.
                                // index: 0 for bank, 1 for program.

    ProcessEvents,              // ptr: VstEvents*.

    CanBeAutomated,             // index: parameter index.
                                // return: 1=true, 0=false.
    StringToParameter,           // index: parameter index.
                                // ptr:   parameter string.
                                // return value: 1 for success.

    _GetNumProgramCategories,   // Deprecated in VST 2.4.

    GetPresetNameIndexed,      // index: program index.
                                // ptr: char[kVstMaxProgNameLen].
                                // return value: 1 for success.

    _CopyProgram,               // Deprecated in VST 2.4.
    _ConnectInput,              // Deprecated in VST 2.4.
    _ConnectOutput,             // Deprecated in VST 2.4.

    GetInputProperties,         // index: input index.
                                // ptr: VstPinProperties*.
                                // return value: 1 if supported.
    GetOutputProperties,        // index: output index.
                                // ptr: VstPinProperties*.
                                // return value: 1 if supported.
    GetPlugCategory,            // return value: PluginCategory.

    _GetCurrentPosition,        // Deprecated in VST 2.4.
    _GetDestinationBuffer,      // Deprecated in VST 2.4.

    OfflineNotify,              // ptr: VstAudioFile*.
                                // value: amount of files.
                                // index: start flag.
    OfflinePrepare,             // ptr: VstOfflineTask*.
                                // value: amount of tasks.  
    OfflineRun,                 // ptr: VstOfflineTask*.
                                // value: amount of tasks.

    ProcessVarIo,               // ptr: VstVariableIo*.  
    SetSpeakerArrangement,      // value: input VstSpeakerArrangement*.
                                // ptr:   output VstSpeakerArrangement*.

    _SetBlockSizeAndSampleRate, // Deprecated in VST 2.4.

    SetBypass,                  // value: 1 = bypass, 0 = no bypass.
    GetPluginName,              // ptr: char[kVstMaxEffectNameLen].

    _GetErrorText,              // Deprecated in VST 2.4.

    GetAuthorName,             // ptr: char[kVstMaxVendorStrLen].
    GetProductName,             // ptr: char[kVstMaxProductStrLen].
    GetProductVersion,          // return value: vendor-specific version.
    VendorSpecific,             // No definition, vendor specific handling.
    CanDo,                      // ptr: "can do" string
                                // return value: 0: "don't know" -1: "no" 1: "yes"  
    GetTailSize,                // return value: tail size (for example the reverb time of a reverb plug-in)
                                // 0 is default (return 1 for 'no tail')

    _Idle,                      // Deprecated in VST 2.4.
    _GetIcon,                   // Deprecated in VST 2.4.
    _SetViewPosition,           // Deprecated in VST 2.4.

    GetParameterProperties,     // index: parameter index.
                                // ptr: VstParameterProperties*.
                                // return value: 1 if supported.

    _KeysRequired,              // Deprecated in VST 2.4.

    GetVstVersion,              // return value: VST version.
    NotifyKeyDown,                // index: ASCII character.
                                // value: virtual key-.
                                // opt: modifiers.
                                // return value: 1 if key used.
    NotifyKeyUp,                  // index: ASCII character.
                                // value: virtual key.
                                // opt: modifiers.
                                // return value: 1 if key used.
    SetEditKnobMode,            // value: knob mode.
                                //  * 0: circular,
                                //  * 1: circular relative,
                                //  * 2: linear (CKnobMode in VSTGUI).

    GetMidiProgramName,         // index: MIDI channel.
                                // ptr: MidiProgramName*.
                                // return value: number of used programs, 0 if unsupported.
    GetCurrentMidiProgram,      // index: MIDI channel.
                                // ptr: MidiProgramName*.
                                // return value: index of current program.
    GetMidiProgramCategory,     // index: MIDI channel.
                                // ptr: MidiProgramCategory*.
                                // return value: number of used categories, 0 if unsupported.
    HasMidiProgramsChanged,     // index: MIDI channel.
                                // return value: 1 if the MidiProgramName or MidiKeyName have changed.
    GetMidiKeyName,             // index: MIDI channel.
                                // ptr: MidiKeyName*.
                                // return value: true if supported, false otherwise.

    BeginSetProgram,            // No arguments.
    EndSetProgram,              // No arguments.

    GetSpeakerArrangement,      // value: input #VstSpeakerArrangement*.
                                // ptr: output #VstSpeakerArrangement*.
    ShellGetNextPlugin,         // ptr: plug-in name char[kVstMaxProductStrLen].
                                // return value: next plugin's unique_id.

    StartProcess,               // No arguments.
    StopProcess,                // No arguments.
    SetTotalSampleToProcess,    // value: number of samples to process,
                                //        offline only.
    SetPanLaw,                  // value: pan law
                                // opt: gain  

    BeginLoadBank,              // ptr: VstPatchChunkInfo*.
                                // return value:
                                //  * -1: bank can't be loaded,
                                //  *  1: bank can be loaded,
                                //  *  0: unsupported.
    BeginLoadProgram,           // ptr: VstPatchChunkInfo*.
                                // return value:
                                //  * -1: program can't be loaded,
                                //  *  1: program can be loaded,
                                //  *  0: unsupported  

    SetProcessPrecision,        // value: VstProcessPrecision.
    GetNumberOfMidiInputChannels,    // return value: number of used MIDI input channels (1-15).
    GetNumberOfMidiOutputChannels,   // return value: number of used MIDI output channels (1-15).
    SetTempo = 65536,           // Used in FL Studio.
                                // value: time per beat in samples?
                                // opt: tempo

    __Size,
};

constexpr char const* host_opcode_string(HostOpcode op)
{
    using enum HostOpcode;
    switch (op) {
    case Automate:
        return "Automate";
    case VstVersion:
        return "VstVersion";
    case CurrentId:
        return "CurrentId";
    case Idle:
        return "Idle";
    case _PinConnected:
        return "_PinConnected";
    case _WantMidi:
        return "_WantMidi";
    case GetTime:
        return "GetTime";
    case ProcessEvents:
        return "ProcessEvents";
    case _SetTime:
        return "_SetTime";
    case _TempoAt:
        return "_TempoAt";
    case _GetNumAutomatableParameters:
        return "_GetNumAutomatableParameters";
    case _GetParameterQuantization:
        return "_GetParameterQuantization";
    case IOChanged:
        return "IOChanged";
    case _NeedIdle:
        return "_NeedIdle";
    case ResizeWindow:
        return "ResizeWindow";
    case GetSampleRate:
        return "GetSampleRate";
    case GetBlockSize:
        return "GetBlockSize";
    case GetInputLatency:
        return "GetInputLatency";
    case GetOutputLatency:
        return "GetOutputLatency";
    case _GetPreviousPlug:
        return "_GetPreviousPlug";
    case _GetNextPlug:
        return "_GetNextPlug";
    case _WillReplaceOrAccumulate:
        return "_WillReplaceOrAccumulate";
    case GetCurrentProcessLevel:
        return "GetCurrentProcessLevel";
    case GetAutomationState:
        return "GetAutomationState";
    case OfflineStart:
        return "OfflineStart";
    case OfflineRead:
        return "OfflineRead";
    case OfflineWrite:
        return "OfflineWrite";
    case OfflineGetCurrentPass:
        return "OfflineGetCurrentPass";
    case OfflineGetCurrentMetaPass:
        return "OfflineGetCurrentMetaPass";
    case _SetOutputSampleRate:
        return "_SetOutputSampleRate";
    case _GetOutputSpeakerArrangement:
        return "_GetOutputSpeakerArrangement";
    case GetAuthorName:
        return "GetAuthorName";
    case GetHostName:
        return "GetHostName";
    case GetVendorVersion:
        return "GetVendorVersion";
    case VendorSpecific:
        return "VendorSpecific";
    case _SetIcon:
        return "_SetIcon";
    case CanDo:
        return "CanDo";
    case GetLanguage:
        return "GetLanguage";
    case _OpenWindow:
        return "_OpenWindow";
    case _CloseWindow:
        return "_CloseWindow";
    case GetDirectory:
        return "GetDirectory";
    case UpdateDisplay:
        return "UpdateDisplay";
    case BeginEdit:
        return "BeginEdit";
    case EndEdit:
        return "EndEdit";
    case OpenFileSelector:
        return "OpenFileSelector";
    case CloseFileSelector:
        return "CloseFileSelector";
    case _EditFile:
        return "_EditFile";
    case _GetChunkFile:
        return "_GetChunkFile";
    case _GetInputSpeakerArrangement:
        return "_GetInputSpeakerArrangement";
    }
}

constexpr char const* plugin_opcode_string(PluginOpcode op)
{
    using enum PluginOpcode;
    switch(op) {
    case Create:
        return "Create";
    case Destroy:
        return "Destroy";
    case SetPresetNumber:
        return "SetPresetNumber";
    case GetPresetNumber:
        return "GetPresetNumber";
    case SetPresetName:
        return "SetPresetName";
    case GetPresetName:
        return "GetPresetName";
    case GetParameterLabel:
        return "GetParameterLabel";
    case GetParameterDisplay:            
        return "GetParameterDisplay";
    case GetParameterName:               
        return "GetParameterName";
    case _GetVu:                     
        return "_GetVu";
    case SetSampleRate:              
        return "SetSampleRate";
    case SetBlockSize:               
        return "SetBlockSize";
    case MainsChanged:               
        return "MainsChanged";
    case GetEditorRectangle:                
        return "GetEditorRectangle";
    case CreateEditor:                   
        return "CreateEditor";
    case DestroyEditor:                  
        return "DestroyEditor";
    case _EditDraw:                  
        return "_EditDraw";
    case _EditMouse:                 
        return "_EditMouse";
    case _EditKey:                   
        return "_EditKey";
    case EditorIdle:                   
        return "EditIdle";
    case _EditTop:                   
        return "_EditTop";
    case _EditSleep:                 
        return "_EditSleep";
    case _Identify:                  
        return "_Identify";
    case GetChunk:                   
        return "GetChunk";
    case SetChunk:
        return "SetChunk";
    case ProcessEvents:              
        return "ProcessEvents";
    case CanBeAutomated:
        return "CanBeAutomated";
    case StringToParameter:
        return "StringToParameter";
    case _GetNumProgramCategories:   
        return "_GetNumProgramCategories";
    case GetPresetNameIndexed:
        return "GetPresetNameIndexed";
    case _CopyProgram:               
        return "_CopyProgram";
    case _ConnectInput:              
        return "_ConnectInput";
    case _ConnectOutput:             
        return "_ConnectOutput";
    case GetInputProperties:         
        return "GetInputProperties";
    case GetOutputProperties:        
        return "GetOutputProperties";
    case GetPlugCategory:            
        return "GetPlugCategory";
    case _GetCurrentPosition:        
        return "_GetCurrentPosition";
    case _GetDestinationBuffer:      
        return "_GetDestinationBuffer";
    case OfflineNotify:              
        return "OfflineNotify";
    case OfflinePrepare:             
        return "OfflinePrepare";
    case OfflineRun:                 
        return "OfflineRun";
    case ProcessVarIo:               
        return "ProcessVarIo";
    case SetSpeakerArrangement:      
        return "SetSpeakerArrangement";
    case _SetBlockSizeAndSampleRate: 
        return "_SetBlockSizeAndSampleRate";
    case SetBypass:                  
        return "SetBypass";
    case GetPluginName:              
        return "GetPluginName";
    case _GetErrorText:              
        return "_GetErrorText";
    case GetAuthorName:            
        return "GetAuthorName";
    case GetProductName:           
        return "GetProductName";
    case GetProductVersion:           
        return "GetProductVersion";
    case VendorSpecific:             
        return "VendorSpecific";
    case CanDo:                      
        return "CanDo";
    case GetTailSize:                
        return "GetTailSize";
    case _Idle:                      
        return "_Idle";
    case _GetIcon:                   
        return "_GetIcon";
    case _SetViewPosition:           
        return "_SetViewPosition";
    case GetParameterProperties:     
        return "GetParameterProperties";
    case _KeysRequired:              
        return "_KeysRequired";
    case GetVstVersion:              
        return "GetVstVersion";
    case NotifyKeyDown:                
        return "EditKeyDown";
    case NotifyKeyUp:                  
        return "EditKeyUp";
    case SetEditKnobMode:            
        return "SetEditKnobMode";
    case GetMidiProgramName:         
        return "GetMidiProgramName";
    case GetCurrentMidiProgram:      
        return "GetCurrentMidiProgram";
    case GetMidiProgramCategory:     
        return "GetMidiProgramCategory";
    case HasMidiProgramsChanged:     
        return "HasMidiProgramsChanged";
    case GetMidiKeyName:             
        return "GetMidiKeyName";
    case BeginSetProgram:            
        return "BeginSetProgram";
    case EndSetProgram:              
        return "EndSetProgram";
    case GetSpeakerArrangement:      
        return "GetSpeakerArrangement";
    case ShellGetNextPlugin:         
        return "ShellGetNextPlugin";
    case StartProcess:               
        return "StartProcess";
    case StopProcess:                
        return "StopProcess";
    case SetTotalSampleToProcess:
        return "SetTotalSampleToProcess";
    case SetPanLaw:
        return "SetPanLaw";
    case BeginLoadBank:
        return "BeginLoadBank";
    case BeginLoadProgram:
        return "BeginLoadProgram";
    case SetProcessPrecision:
        return "SetProcessPrecision";
    case GetNumberOfMidiInputChannels:
        return "GetNumMidiInputChannels";
    case GetNumberOfMidiOutputChannels:
        return "GetNumMidiOutputChannels";
    case SetTempo:
        return "SetTempo";
    case __Size:
        return nullptr;
    }
    return nullptr;
}

}
