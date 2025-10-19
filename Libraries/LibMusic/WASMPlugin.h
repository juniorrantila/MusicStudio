#pragma once
#include <LibCore/MappedFile.h>
#include <LibTy/StringBuffer.h>
#include <WASM3/wasm3.h>

namespace MS {

struct WASMPluginManager;
struct WASMPlugin {
    WASMPluginManager* manager;

    StringView fallback_name {};
    IM3Environment env { nullptr };
    IM3Runtime runtime { nullptr };
    IM3Module mod { nullptr };
    struct {
        IM3Function init { nullptr };
        IM3Function deinit { nullptr };
        IM3Function process_f32 { nullptr };
        IM3Function process_f64 { nullptr };

        IM3Function parameter_count { nullptr };
        IM3Function get_parameter { nullptr };
        IM3Function set_parameter { nullptr };
        IM3Function parameter_kind { nullptr };
        IM3Function parameter_name { nullptr };
        IM3Function parameter_min_value { nullptr };
        IM3Function parameter_max_value { nullptr };
        IM3Function parameter_step_size{ nullptr };
        IM3Function parameter_option_name { nullptr };

    } func;
    mutable u64 heap_base { 0 };
    mutable u64 heap_end { 0 };

    WASMPlugin(WASMPlugin&&);
    WASMPlugin& operator=(WASMPlugin&&);
    ~WASMPlugin();

    static ErrorOr<WASMPlugin> create(WASMPluginManager* manager, StringView name, Bytes bytes);

    StringView name() const;
    ErrorOr<void> link();
    ErrorOr<void> init() const;
    ErrorOr<void> deinit() const;

    ErrorOr<void> process_f32(f32* out, f32 const* in, u32 frames, u32 channels) const;
    ErrorOr<void> process_f64(f64* out, f64 const* in, u32 frames, u32 channels) const;

    bool can_process_f32() const;
    bool can_process_f64() const;

    ErrorOr<u32> parameter_count() const;
    ErrorOr<f64> parameter(u32 id) const;
    ErrorOr<void> set_parameter(u32 id, f64 value) const;
    ErrorOr<StringView> parameter_kind(u32 id) const;
    ErrorOr<StringView> parameter_name(u32 id) const;
    ErrorOr<f64> parameter_min_value(u32 id) const;
    ErrorOr<f64> parameter_max_value(u32 id) const;
    ErrorOr<f64> parameter_step_size(u32 id) const;
    ErrorOr<StringView> parameter_option_name(u32 id, u32 option) const;

    Optional<u64> wasm_alloc(u64 size) const;
    void wasm_free(u64 size) const;

    ErrorOr<u64> wasm_memset(u64 wasm_dest, int c, usize size) const;
    ErrorOr<u64> wasm_memcpy(u64 wasm_dest, void const* from, usize size) const;
    ErrorOr<void*> wasm_memcpy(void* dest, u64 wasm_from, usize size) const;

    Optional<void*> wasm_resolve_ptr(u64 ptr) const;
    Optional<StringView> wasm_resolve_c_string(u64 ptr) const;
    Optional<StringView> wasm_resolve_string(u64 ptr, u64 size) const;

private:
    WASMPlugin(WASMPluginManager* manager, StringView name, IM3Environment env, IM3Runtime runtime, IM3Module mod);
};

}
