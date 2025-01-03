#pragma once
#include <Core/MappedFile.h>
#include <Ty/StringBuffer.h>
#include <WASM3/wasm3.h>

namespace MS {

struct WASMPlugin {
    Core::MappedFile file;
    StringBuffer fallback_name {};
    IM3Environment env { nullptr };
    IM3Runtime runtime { nullptr };
    IM3Module mod { nullptr };
    IM3Function main { nullptr };
    IM3Function process_f32_func { nullptr };
    IM3Function process_f64_func { nullptr };

    WASMPlugin(WASMPlugin&&);
    WASMPlugin& operator=(WASMPlugin&&);
    ~WASMPlugin();

    static ErrorOr<WASMPlugin> create(StringView path);

    StringView name() const;
    ErrorOr<void> link();
    ErrorOr<void> run() const;

    ErrorOr<void> process_f32(f32* out, f32 const* in, u32 frames, u32 channels) const;
    ErrorOr<void> process_f64(f64* out, f64 const* in, u32 frames, u32 channels) const;

    bool can_process_f32() const;
    bool can_process_f64() const;

private:
    WASMPlugin(Core::MappedFile&& file, StringBuffer&& name, IM3Environment env, IM3Runtime runtime, IM3Module mod);
};

}
