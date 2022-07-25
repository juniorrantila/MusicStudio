#pragma once
#include <JR/ErrorOr.h>
#include <Vst/Rectangle.h>
#include <Vst/Vst.h>
#include <Host.h>
#include <Core/Library.h>

struct Plugin {
    Core::Library plugin_library;
    Vst::Effect* vst;
    Host* host;

    static ErrorOr<Plugin> create_from(char const* path);
    void destroy() const;
    bool is_valid() const { return plugin_library.is_valid() && vst && host; }

    // FIXME: Cache this.
    ErrorOr<StringView> name() const
    {
        auto name = vst->name();
        if (!name)
            return Error::from_string_literal("could not get plugin name");
        return StringView(name);
    }

    // FIXME: Cache this.
    ErrorOr<StringView> author() const
    {
        auto author = vst->author();
        if (!author)
            return Error::from_string_literal("could not get plugin author");
        return StringView(author);
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

    Vst::Rectangle editor_rectangle() const
    {
        return vst->editor_rectangle();
    }

    ErrorOr<void> open_editor(void* raw_handle) const
    {
        if (!vst->open_editor(raw_handle))
            return Error::from_string_literal("could not open editor");
        return {};
    }

    void close_editor() const
    {
        (void)vst->close_editor();
    }

#if 0
    ErrorOr<void> update_sample_rate() const
    {
        // jack_get_sample_rate
        vst->set_sample_rate(host->sample_rate);
    }
#endif

    ErrorOr<void> set_sample_rate(f32 sample_rate) const
    {
        if (!vst->set_sample_rate(sample_rate))
            return Error::from_string_literal("could not set sample rate");
        return {};
    }

    ErrorOr<void> pause() const
    {
        if (!vst->pause())
            return Error::from_string_literal("could not pause plugin");
        return {};
    }

    ErrorOr<void> resume() const
    {
        if (!vst->resume())
            return Error::from_string_literal("could not resume plugin");
        return {};
    }

private:
    void invalidate()
    {
        vst = nullptr;
        host = nullptr;
    }
};

