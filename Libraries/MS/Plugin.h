#pragma once
#include "Host.h"

#include <Ty/ErrorOr.h>
#include <Vst/Rectangle.h>
#include <Vst/Vst.h>
#include <Core/Library.h>

namespace MS {

struct Plugin {
    Core::Library plugin_library;
    Vst::Effect* vst;
    Host* host;

    static ErrorOr<Plugin> create_from(char const* path);
    void destroy() const;
    bool is_valid() const { return plugin_library.is_valid() && vst && host; }

    void process_f32(f32* const* outputs, f32 const* const* inputs, i32 samples) const
    {
        if (vst->process_f32) {
            vst->process_f32(vst, inputs, outputs, samples);
        } else {
            vst->_process(vst, inputs, outputs, samples);
        }
    }

    void process_f64(f64* const* outputs, f64 const* const* inputs, i32 samples) const
    {
        vst->process_f64(vst, inputs, outputs, samples);
    }

    // FIXME: Cache this.
    Optional<StringView> name() const
    {
        auto name = vst->name();
        if (!name)
            return {};
        return StringView::from_c_string(name);
    }

    // FIXME: Cache this.
    Optional<StringView> author() const
    {
        auto author = vst->author();
        if (!author)
            return {};
        return StringView::from_c_string(author);
    }

    u32 product_version() const
    {
        return vst->product_version();
    }

    u32 version() const
    {
        return vst->version;
    }

    u32 vst_version() const
    {
        return vst->vst_version();
    }

    u32 vst_magic() const
    {
        return vst->vst_magic;
    }

    u32 number_of_parameters() const
    {
        return vst->number_of_parameters;
    }

    u32 number_of_presets() const
    {
        return vst->number_of_presets;
    }

    u32 number_of_inputs() const
    {
        return vst->number_of_inputs;
    }

    u32 number_of_outputs() const
    {
        return vst->number_of_outputs;
    }

    bool has_editor() const
    {
        return vst->has_editor();
    }

    bool supports_f32() const
    {
        return vst->supports_f32();
    }

    bool supports_f64() const
    {
        return vst->supports_f64();
    }

    bool uses_program_chunks() const
    {
        return vst->uses_program_chunks();
    }

    bool is_synth() const
    {
        return vst->is_synth();
    }

    bool is_silent_when_stopped() const
    {
        return vst->is_silent_when_stopped();
    }

    Optional<Vst::Rectangle> editor_rectangle() const
    {
        return vst->editor_rectangle();
    }

    [[nodiscard]] bool open_editor(void* raw_handle) const
    {
        return vst->open_editor(raw_handle);
    }

    [[nodiscard]] bool close_editor() const
    {
        return vst->close_editor();
    }

    [[nodiscard]] bool set_sample_rate(f32 sample_rate) const
    {
        return vst->set_sample_rate(sample_rate);
    }

    [[nodiscard]] bool pause() const
    {
        return vst->pause();
    }

    [[nodiscard]] bool resume() const
    {
        return vst->resume();
    }

private:
    void invalidate()
    {
        vst = nullptr;
        host = nullptr;
    }
};

}
