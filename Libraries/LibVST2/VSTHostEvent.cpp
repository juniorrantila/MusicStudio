#include "./VSTHostEvent.h"
#include "./Language.h"

C_API bool vst_host_automate(VSTHostDispatcher dispatcher, i32 parameter_index, f32 value)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_Automate, parameter_index, 0, 0, value) == 1;
}

C_API iptr vst_host_vst_version(VSTHostDispatcher dispatcher)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_VstVersion, 0, 0, 0, 0);
}

C_API iptr vst_host_current_id(VSTHostDispatcher dispatcher)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_CurrentId, 0, 0, 0, 0);
}

C_API bool vst_host_idle(VSTHostDispatcher dispatcher)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_Idle, 0, 0, 0, 0) == 1;
}

C_API VSTTimeInfo* vst_host_time(VSTHostDispatcher dispatcher)
{
    return (VSTTimeInfo*)dispatcher.dispatch(dispatcher.effect, VSTHostEvent_GetTime, 0, 0, 0, 0);
}

C_API bool vst_host_process_events(VSTHostDispatcher dispatcher, VSTEvents* events)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_ProcessEvents, 0, 0, events, 0) == 1;
}

C_API bool vst_host_io_changed(VSTHostDispatcher dispatcher)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_IOChanged, 0, 0, 0, 0) == 1;
}

C_API bool vst_host_resize_window(VSTHostDispatcher dispatcher, i32 width, i32 height)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_ResizeWindow, width, height, 0, 0) == 1;
}

C_API iptr vst_host_sample_rate(VSTHostDispatcher dispatcher)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_GetSampleRate, 0, 0, 0, 0);
}

C_API iptr vst_host_block_size(VSTHostDispatcher dispatcher)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_GetBlockSize, 0, 0, 0, 0);
}

C_API iptr vst_host_input_latency(VSTHostDispatcher dispatcher)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_GetInputLatency, 0, 0, 0, 0);
}

C_API iptr vst_host_output_latency(VSTHostDispatcher dispatcher)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_GetOutputLatency, 0, 0, 0, 0);
}

C_API iptr vst_host_current_process_level(VSTHostDispatcher dispatcher)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_GetCurrentProcessLevel, 0, 0, 0, 0);
}

C_API iptr vst_host_automation_state(VSTHostDispatcher dispatcher)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_GetAutomationState, 0, 0, 0, 0);
}

C_API iptr vst_host_author_name(VSTHostDispatcher dispatcher, char* buf)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_GetAuthorName, 0, 0, buf, 0);
}

C_API iptr vst_host_host_name(VSTHostDispatcher dispatcher, char* buf)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_GetHostName, 0, 0, buf, 0);
}

C_API iptr vst_host_vendor_version(VSTHostDispatcher dispatcher)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_GetVendorVersion, 0, 0, 0, 0);
}

C_API iptr vst_host_vendor_specific(VSTHostDispatcher dispatcher, i32 index, iptr value, void* ptr, f32 opt)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_VendorSpecific, index, value, ptr, opt);
}

C_API iptr vst_host_can_do(VSTHostDispatcher dispatcher, char const* ptr)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_CanDo, 0, 0, (void*)ptr, 0);
}

VSTLanguage vst_host_language(VSTHostDispatcher dispatcher)
{
    iptr language = dispatcher.dispatch(dispatcher.effect, VSTHostEvent_GetLanguage, 0, 0, 0, 0);
    switch (language) {
    case VSTLanguage_English: return VSTLanguage_English;
    case VSTLanguage_German: return VSTLanguage_German;
    case VSTLanguage_French: return VSTLanguage_French;
    case VSTLanguage_Italian: return VSTLanguage_Italian;
    case VSTLanguage_Spanish: return VSTLanguage_Spanish;
    case VSTLanguage_Japanese: return VSTLanguage_Japanese;
    }
    return VSTLanguage_Unknown;
}

#if __APPLE__
C_API FSSpec* vst_host_directory(VSTHostDispatcher dispatcher)
{
    return (FSSpec*)dispatcher.dispatch(dispatcher.effect, VSTHostEvent_GetDirectory, 0, 0, 0, 0);
}
#else
C_API char* vst_host_directory(VSTHostDispatcher dispatcher)
{
    return (char*)dispatcher.dispatch(dispatcher.effect, VSTHostEvent_GetDirectory, 0, 0, 0, 0);
}
#endif

C_API bool vst_host_update_display(VSTHostDispatcher dispatcher)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_UpdateDisplay, 0, 0, 0, 0) == 1;
}

C_API bool vst_host_begin_edit(VSTHostDispatcher dispatcher, i32 parameter)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_BeginEdit, parameter, 0, 0, 0) == 1;
}

C_API bool vst_host_end_edit(VSTHostDispatcher dispatcher, i32 parameter)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_EndEdit, parameter, 0, 0, 0) == 1;
}

C_API bool vst_host_open_file_selector(VSTHostDispatcher dispatcher, VSTFileSelect* ptr)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_OpenFileSelector, 0, 0, ptr, 0) == 1;
}

C_API bool vst_host_close_file_selector(VSTHostDispatcher dispatcher, VSTFileSelect* ptr)
{
    return dispatcher.dispatch(dispatcher.effect, VSTHostEvent_CloseFileSelector, 0, 0, ptr, 0) == 1;
}
