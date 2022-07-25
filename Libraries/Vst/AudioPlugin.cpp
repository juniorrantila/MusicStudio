#include <JR/Log.h>
#include <Vst/AudioPlugin.h>
#include <Vst/Opcodes.h>

#include <time.h>
#include <stdio.h>

namespace Vst {

bool AudioPlugin::process_events(Events* events)
{
    auto next_event = events->events[0];
    for (i32 i = 0; i < events->number_of_events; i++) {
        auto generic_event = next_event;
        next_event = generic_event + generic_event->full_size();

        using enum EventType;
        switch (generic_event->type) {
        case Midi: {
            process_midi_event((MidiEvent*)generic_event);
#if 0
            LOG("midi delta frames %d", event->delta_frames);
            LOG("midi detune cents %d", event->detune_cents);
            LOG("midi note length in samples %d",
                event->note_length_in_samples);
            LOG("midi note offset in samples %d",
                event->note_offset_in_samples);
            LOG("midi note off velocity %d",
                event->note_off_velocity);
#endif
            break;
        }
        case _Audio: {
            LOG("audio event");
            break;
        }
        case _Video: {
            LOG("video event");
            break;
        }
        case _Parameter: {
            LOG("parameter event");
            break;
        }
        case _Trigger: {
            LOG("trigger event");
            break;
        }
        case MidiSystemExclusive: {
#if 0
            auto event = (MidiSystemExclusiveEvent*)generic_event;
            LOG("midi system exclusive event [size: %d]", event->system_exclusive_dump_size);
            LOG("midi delta frames %d", event->delta_frames);
            for (i32 i = 0; i < event->system_exclusive_dump_size; i++) {
                LOG("midi dump[%d] %.2x", i,
                    event->system_exclusive_dump[i]);
            }
#endif
            break;
        }
        }
    }
    return false;
}

static char* vst_strncpy(char* dest, char const* source, u32 size)
{
    auto ret = __builtin_strncpy(dest, source, size);
    dest[size - 1] = '\0';
    return ret;
}

intptr_t AudioPlugin::dispatch(PluginOpcode opcode, i32 index, intptr_t value,
    void* ptr, f32 opt)
{
    switch (opcode) {
    default: // FIXME
        return 0;

    case PluginOpcode::Create: // Created elsewhere.
        __builtin_unreachable();
        return 0;

    case PluginOpcode::Destroy: // Destroyed elsewhere.
        __builtin_unreachable();
        return 0;

    case PluginOpcode::SetPresetNumber:
        return set_current_preset((i32)value);

    case PluginOpcode::GetPresetNumber:
        return current_preset();

    case PluginOpcode::SetPresetName:
        return set_preset_name((char const*)ptr);

    case PluginOpcode::GetPresetName: {
        auto name = preset_name();
        if (!name)
            return 0;
        vst_strncpy((char*)ptr, name, MAX_NAME_LENGTH);
        return 1;
    }

    case PluginOpcode::GetParameterLabel: {
        auto name = parameter_label(index);
        if (!name)
            return 0;
        vst_strncpy((char*)ptr, name, MAX_LONG_LABEL_LENGTH);
        return 1;
    }

    case PluginOpcode::GetParameterDisplay: {
        auto name = parameter_display(index);
        if (!name)
            return 0;
        vst_strncpy((char*)ptr, name, MAX_LONG_LABEL_LENGTH);
        return 1;
    }

    case PluginOpcode::GetParameterName: {
        auto name = parameter_name(index);
        if (!name)
            return 0;
        vst_strncpy((char*)ptr, name, MAX_LONG_LABEL_LENGTH);
        return 1;
    }

    case PluginOpcode::SetSampleRate:
        return set_sample_rate(opt);

    case PluginOpcode::SetBlockSize:
        return set_block_size((i32)value);

    case PluginOpcode::MainsChanged:
        if (value)
            return resume();
        return suspend();

    case PluginOpcode::_GetVu:
        // return vu() * 32767.0;
        return 0;

    case PluginOpcode::GetEditorRectangle: {
        auto const* rectangle = editor_rectangle();
        if (!rectangle)
            return 0;
        *((Rectangle const**)ptr) = rectangle;
        return 1;
    }

    case PluginOpcode::CreateEditor:
        return open_editor(ptr);

    case PluginOpcode::DestroyEditor:
        close_editor();
        return 1;

    case PluginOpcode::EditorIdle:
        return 0;

#if 0 // Deprecated.
#    if (TARGET_API_MAC_CARBON && !VST_FORCE_DEPRECATED)
        case effEditDraw:            if (editor) editor->draw ((ERect*)ptr);                break;
        case effEditMouse:            if (editor) v = editor->mouse (index, value);        break;
        case effEditKey:            if (editor) v = editor->key (value);                break;
        case effEditTop:            if (editor) editor->top ();                            break;
        case effEditSleep:            if (editor) editor->sleep ();                        break;
#    endif
#endif

    case PluginOpcode::_Identify:
        return config().author_id;

    case PluginOpcode::GetChunk: {
        // FIXME:
        return 0;
#if 0
        auto chunk = get_chunk((ChunkType)index);
        if (!chunk)
            return 0;
        // memcpy(ptr, chunk, chunk_size);
        return 1;
#endif
    }

    case PluginOpcode::SetChunk:
        return set_chunk((ChunkType)index,
            value, ptr);

    case PluginOpcode::ProcessEvents:
        return process_events((Events*)ptr);

    case PluginOpcode::CanBeAutomated:
        return parameter_can_be_automated(index);

    // FIXME:
    case PluginOpcode::StringToParameter:
        return 0;
        // return string_to_parameter(index, (char*)ptr);

    case PluginOpcode::GetPresetNameIndexed: {
        auto const* name = preset_name_from_id(index);
        if (!name)
            return 0;
        vst_strncpy((char*)ptr, name, MAX_NAME_LENGTH);
        return 1;
    }

#if 0
    case PluginOpcode::_GetNumProgramCategories:
        return 0;

        case PluginOpcode::CopyProgram:
            return copy_program(index)

        case PluginOpcode::ConnectInput:
            input_connected(index, value ? true : false);
            v = 1;
            break;

        case effConnectOutput:
            outputConnected (index, value ? true : false);
            v = 1;
            break;
#endif

    case PluginOpcode::GetInputProperties: {
        // FIXME: Make this fallible
        auto properties = input_properties(index);
        *((PinProperties*)ptr) = properties;
        return 1;
    }

    case PluginOpcode::GetOutputProperties: {
        // FIXME: Make this fallible
        auto properties = output_properties(index);
        *((PinProperties*)ptr) = properties;
        return 1;
    }

    case PluginOpcode::GetPlugCategory:
        return (intptr_t)config().category;

#if 0
#    if !VST_FORCE_DEPRECATED
    //---Realtime----------------------
    case effGetCurrentPosition:
        v = reportCurrentPosition();
        break;

    case effGetDestinationBuffer:
        v = ToVstPtr<float>(reportDestinationBuffer());
        break;
#    endif // !VST_FORCE_DEPRECATED
#endif

    case PluginOpcode::OfflineNotify:
        return notify_offline((AudioFile*)ptr,
            value, index != 0);

    case PluginOpcode::OfflinePrepare:
        return prepare_offline((OfflineTask*)ptr,
            value);

    case PluginOpcode::OfflineRun:
        return run_offline((OfflineTask*)ptr, value);

    // FIXME: Variable size speaker arrangement
    case PluginOpcode::SetSpeakerArrangement: {
        return set_speaker_arrangement(
            (SpeakerArrangement*)ptr);
    }

    case PluginOpcode::ProcessVarIo:
        return process_variable_io((VariableIO*)ptr);

#if 0
#    if !VST_FORCE_DEPRECATED
    case effSetBlockSizeAndSampleRate:
        setBlockSizeAndSampleRate((VstInt32)value, opt);
        v = 1;
        break;
#    endif
#endif

    case PluginOpcode::SetBypass:
        return set_bypass(value);

    case PluginOpcode::GetPluginName: {
        vst_strncpy((char*)ptr, config().plugin_name, MAX_NAME_LENGTH);
        return 1;
    }

    case PluginOpcode::GetAuthorName: {
        vst_strncpy((char*)ptr, config().author_name, MAX_NAME_LENGTH);
        return 1;
    }

    case PluginOpcode::GetProductName: {
        vst_strncpy((char*)ptr, config().plugin_name, MAX_NAME_LENGTH);
        return 1;
    }

    case PluginOpcode::GetProductVersion:
        return config().plugin_version;

    case PluginOpcode::VendorSpecific:
        return vendor_specific(index, value,
            ptr, opt);
    case PluginOpcode::CanDo:
        return (intptr_t)can_do((char*)ptr);

    case PluginOpcode::GetTailSize:
        return tail_size();

#if 0
#    if !VST_FORCE_DEPRECATED
    case effGetErrorText:
        v = getErrorText((char*)ptr) ? 1 : 0;
        break;

    case effGetIcon:
        v = ToVstPtr<void>(getIcon());
        break;

    case effSetViewPosition:
        v = setViewPosition(index, (VstInt32)value) ? 1 : 0;
        break;

    case effIdle:
        v = fxIdle();
        break;

    case effKeysRequired:
        v = (keysRequired() ? 0 : 1); // reversed to keep v1 compatibility
        break;
#    endif // !VST_FORCE_DEPRECATED
#endif

    case PluginOpcode::GetParameterProperties: {
        // FIXME: Make this fallible
        auto prop = parameter_properties(index);
        *((ParameterProperties*)ptr) = prop;
        return 1;
    }

    case PluginOpcode::GetVstVersion:
        return config().vst_version;

#if 0 // FIXME: Editor
    case effEditKeyDown:
        if (editor) {
            VstKeyCode keyCode = { index, (unsigned char)value, (unsigned char)opt };
            v = editor->onKeyDown(keyCode) ? 1 : 0;
        }
        break;

    case effEditKeyUp:
        if (editor) {
            VstKeyCode keyCode = { index, (unsigned char)value, (unsigned char)opt };
            v = editor->onKeyUp(keyCode) ? 1 : 0;
        }
        break;

#endif
    case PluginOpcode::SetEditKnobMode:
        return set_knob_mode((KnobMode)value);

#if 0
    case PluginOpcode::GetMidiProgramName: {
        auto name = midi_preset_name(index);
        if (!name)
            return 0;
        *((MidiPresetName*)ptr) = *name;
        return 1;
    }

    case PluginOpcode::GetCurrentMidiProgram:
        v = getCurrentMidiProgram(index, (MidiProgramName*)ptr);
        break;
    case effGetMidiProgramCategory:
        v = getMidiProgramCategory(index, (MidiProgramCategory*)ptr);
        break;
    case effHasMidiProgramsChanged:
        v = hasMidiProgramsChanged(index) ? 1 : 0;
        break;
#endif
    case PluginOpcode::GetMidiKeyName:
        vst_strncpy((char*)ptr, "noname", 6);
        return 1;
#if 0
    case effBeginSetProgram:
        v = beginSetProgram() ? 1 : 0;
        break;
    case effEndSetProgram:
        v = endSetProgram() ? 1 : 0;
        break;
#endif // VST_2_1_EXTENSIONS

#if 0
    case effGetSpeakerArrangement:
        v = getSpeakerArrangement(FromVstPtr<VstSpeakerArrangement*>(value), (VstSpeakerArrangement**)ptr) ? 1 : 0;
        break;
#endif

    case PluginOpcode::SetTotalSampleToProcess:
        return set_total_samples_to_process(value);

#if 0
    case effShellGetNextPlugin:
        v = getNextShellPlugin((char*)ptr);
        break;
#endif

    case PluginOpcode::StartProcess:
        return start_process();

    case PluginOpcode::StopProcess:
        return stop_process();

    case PluginOpcode::SetPanLaw:
        return set_pan_law((PanLaw)value, opt);

#if 0
    case effBeginLoadBank:
        v = beginLoadBank((VstPatchChunkInfo*)ptr);
        break;
    case effBeginLoadProgram:
        v = beginLoadProgram((VstPatchChunkInfo*)ptr);
        break;
#endif // VST_2_3_EXTENSIONS

    case PluginOpcode::SetProcessPrecision: {
        auto precision = (Precision)value;
        return set_process_precision(precision);
    }

    case PluginOpcode::GetNumberOfMidiInputChannels:
        return midi_input_channels_size();

    case PluginOpcode::GetNumberOfMidiOutputChannels:
        return midi_output_channels_size();

    case PluginOpcode::SetTempo:
        return 1;
    }

    return 0;
}

}
