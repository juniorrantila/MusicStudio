#pragma once
#include "./AEffect.h"
#include "./CanDo.h"
#include "./Opcodes.h"
#include "./Vst.h"

#include <LibTy/Base.h>

namespace Vst {

struct Host {
    constexpr Host(Effect* effect, HostCallback callback)
        : m_effect(effect)
        , m_callback(callback)
    {
    }

    constexpr bool automate(i32 parameter, f32 value) const
    {
        return dispatch(HostOpcode::Automate, parameter, 0, 0, value);
    }

    constexpr u32 vst_version() const
    {
        return dispatch(HostOpcode::VstVersion);
    }

    constexpr i32 current_id() const
    {
        return dispatch(HostOpcode::CurrentId);
    }

    constexpr void idle() const
    {
        dispatch(HostOpcode::Idle);
    }

    TimeInfo const* time() const
    {
        return (TimeInfo const*)dispatch(HostOpcode::GetTime);
    }

    // FIXME: ProcessEvents,                  // ptr: VstEvents*.

    constexpr bool io_changed() const
    {
        return dispatch(HostOpcode::IOChanged);
    }

    constexpr bool resize_window(u32 width, u32 height) const
    {
        return dispatch(HostOpcode::ResizeWindow, width, height);
    }

    constexpr u64 sample_rate() const
    {
        return dispatch(HostOpcode::GetSampleRate);
    }

    constexpr u64 block_size() const
    {
        return dispatch(HostOpcode::GetBlockSize);
    }

    constexpr u64 input_latency_in_samples() const
    {
        return dispatch(HostOpcode::GetInputLatency);
    }

    constexpr u64 output_latency_in_samples() const
    {
        return dispatch(HostOpcode::GetOutputLatency);
    }

    constexpr ProcessLevel process_level() const
    {
        return (ProcessLevel)dispatch(HostOpcode::GetCurrentProcessLevel);
    }

    // GetAutomationState, return value: current automation state.

    // FIXME: OfflineStart // index: new audio files size.
    // value: audio files size.
    // ptr: AudioFile*.

    // FIXME: OfflineRead  // index: bool read_source.
    // value: OfflineOption*.
    // ptr: OfflineTask*.

    // FIXME: OfflineWrite

    // FIXME: OfflineGetCurrentPass

    // FIXME: OfflineGetCurrentMetaPass

    constexpr u32 vendor_version() const
    {
        return dispatch(HostOpcode::GetVendorVersion);
    }

    constexpr CanDo can_do(char const* thing) const
    {
        return (CanDo)dispatch(HostOpcode::CanDo, 0, 0, (void*)thing, 0);
    }

    constexpr HostLanguage language() const
    {
        return (HostLanguage)dispatch(HostOpcode::GetLanguage);
    }

#if __apple__
    FSSpec directory() const
    {
        return (FSSpec)dispatch(HostOpcode::GetLanguage);
    }
#else
    char const* directory() const
    {
        return (char const*)dispatch(HostOpcode::GetDirectory);
    }
#endif

    constexpr void update_display() const
    {
        dispatch(HostOpcode::UpdateDisplay);
    }

    constexpr void begin_edit(i32 parameter) const
    {
        dispatch(HostOpcode::BeginEdit, parameter);
    }

    constexpr void end_edit(i32 parameter) const
    {
        dispatch(HostOpcode::EndEdit, parameter);
    }

    constexpr bool open_file_selector(FileSelect* file_select) const
    {
        return dispatch(HostOpcode::OpenFileSelector, 0, 0, file_select);
    }

    constexpr bool close_file_selector(FileSelect* file_select) const
    {
        return dispatch(HostOpcode::OpenFileSelector, 0, 0, file_select);
    }

    constexpr char const* author()
    {
        if (author_initialized())
            return m_author;
        if (!dispatch(HostOpcode::GetAuthorName, 0, 0, m_author, 0))
            return nullptr;
        return m_author;
    }

    constexpr char const* name()
    {
        if (name_initialized())
            return m_name;
        if (!dispatch(HostOpcode::GetHostName, 0, 0, m_name, 0))
            return nullptr;
        return m_name;
    }

    constexpr iptr vendor_specific(i32 index, iptr value, void* ptr,
        f32 opt) const
    {
        return dispatch(HostOpcode::VendorSpecific, index, value, ptr, opt);
    }

private:
    constexpr iptr dispatch(HostOpcode opcode, i32 index = 0,
        iptr value = 0, void* ptr = nullptr, f32 opt = 0) const
    {
        return m_callback(m_effect, opcode, index, value, ptr, opt);
    }

    constexpr bool author_initialized() const { return m_author[0] != '\0'; }
    constexpr bool name_initialized() const { return m_name[0] != '\0'; }

    Effect* m_effect { nullptr };
    HostCallback m_callback { nullptr };
    char m_author[MAX_NAME_LENGTH] {};
    char m_name[MAX_NAME_LENGTH] {};
};

}
