#include "./WASMPlugin.h"

#include "./WASMPluginManager.h"

#include <WASM3/wasm3.h>
#include <Core/Print.h>
#include <Ty/Defer.h>
#include <Core/Time.h>
#include <string.h>
#include <Ty/Verify.h>
#include <Ty/Assert.h>

namespace MS {

static const void* ms_info(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory);
static const void* ms_debug(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory);
static const void* ms_error(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory);
static const void* ms_fatal(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory);

static const void* ms_time_f32(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory);
static const void* ms_time_f64(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory);

static const void* ms_get_sample_rate(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory);
static const void* ms_get_channels(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory);

static const void* ms_store(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory);
static const void* ms_stores(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory);
static const void* ms_fetch(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory);

static const void* ms_heap_base(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory);
static const void* ms_heap_end(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory);

static bool function_has_signature(IM3Function func, StringView signature);

WASMPlugin::WASMPlugin(WASMPluginManager* manager, Core::MappedFile&& file, StringBuffer&& name, IM3Environment env, IM3Runtime runtime, IM3Module mod)
    : manager(manager)
    , file(move(file))
    , fallback_name(move(name))
    , env(env)
    , runtime(runtime)
    , mod(mod)
{
}

WASMPlugin::WASMPlugin(WASMPlugin&& other)
    : manager(other.manager)
    , file(move(other.file))
    , fallback_name(move(other.fallback_name))
    , env(other.env)
    , runtime(other.runtime)
    , mod(other.mod)
    , func(other.func)
    , heap_base(other.heap_base)
    , heap_end(other.heap_end)
{
    other.env = nullptr;
}

WASMPlugin& WASMPlugin::operator=(WASMPlugin&& other)
{
    if (this == &other) return *this;
    file = move(other.file);
    fallback_name = move(other.fallback_name);
    env = other.env;
    runtime = other.runtime;
    mod = other.mod;
    func = other.func;
    heap_base = other.heap_base;
    heap_end = other.heap_end;
    other.env = nullptr;
    return *this;
}

WASMPlugin::~WASMPlugin()
{
    if (env) {
        m3_FreeEnvironment(env);
        m3_FreeRuntime(runtime);
        env = nullptr;
    }
}

ErrorOr<WASMPlugin> WASMPlugin::create(WASMPluginManager* manager, StringView path)
{
    auto file = TRY(Core::MappedFile::open(path));
    auto path_parts = TRY(path.split_on('/'));
    auto name = TRY(StringBuffer::create_fill(path_parts.last()));

    IM3Environment env = m3_NewEnvironment();
    if (!env) return Error::from_string_literal("could not create WASM environment");
    Defer free_env = [&]{
        m3_FreeEnvironment(env);
    };

    IM3Runtime runtime = m3_NewRuntime(env, 1024, nullptr);
    if (!env) return Error::from_string_literal("could not create WASM runtime");
    Defer free_runtime = [&]{
        m3_FreeRuntime(runtime);
    };

    IM3Module mod;
    if (c_string res = m3_ParseModule(env, &mod, file.data(), file.size())) {
        return Error::from_string_literal(res);
    }

    if (c_string res = m3_LoadModule(runtime, mod)) {
        return Error::from_string_literal(res);
    }

    free_env.disarm();
    free_runtime.disarm();
    return WASMPlugin {
        manager,
        move(file),
        move(name),
        env,
        runtime,
        mod,
    };
}

ErrorOr<void> WASMPlugin::link()
{
    if (c_string res = m3_LinkRawFunctionEx(mod, "ms", "log_info", "v(**)", ms_info, this)) {
        if (res != m3Err_functionLookupFailed) {
            return Error::from_string_literal(res);
        }
    }
    if (c_string res = m3_LinkRawFunctionEx(mod, "ms", "log_debug", "v(**)", ms_debug, this)) {
        if (res != m3Err_functionLookupFailed) {
            return Error::from_string_literal(res);
        }
    }
    if (c_string res = m3_LinkRawFunctionEx(mod, "ms", "log_error", "v(**)", ms_error, this)) {
        if (res != m3Err_functionLookupFailed) {
            return Error::from_string_literal(res);
        }
    }
    if (c_string res = m3_LinkRawFunctionEx(mod, "ms", "log_fatal", "v(**)", ms_fatal, this)) {
        if (res != m3Err_functionLookupFailed) {
            return Error::from_string_literal(res);
        }
    }
    if (c_string res = m3_LinkRawFunctionEx(mod, "ms", "time_f32", "f()", ms_time_f32, this)) {
        if (res != m3Err_functionLookupFailed) {
            return Error::from_string_literal(res);
        }
    }
    if (c_string res = m3_LinkRawFunctionEx(mod, "ms", "time_f64", "F()", ms_time_f64, this)) {
        if (res != m3Err_functionLookupFailed) {
            return Error::from_string_literal(res);
        }
    }

    if (c_string res = m3_LinkRawFunctionEx(mod, "ms", "get_sample_rate", "i()", ms_get_sample_rate, this)) {
        if (res != m3Err_functionLookupFailed) {
            return Error::from_string_literal(res);
        }
    }

    if (c_string res = m3_LinkRawFunctionEx(mod, "ms", "get_channels", "i()", ms_get_channels, this)) {
        if (res != m3Err_functionLookupFailed) {
            return Error::from_string_literal(res);
        }
    }

    if (c_string res = m3_LinkRawFunctionEx(mod, "ms", "store", "i(*i*i)", ms_store, this)) {
        if (res != m3Err_functionLookupFailed) {
            return Error::from_string_literal(res);
        }
    }

    if (c_string res = m3_LinkRawFunctionEx(mod, "ms", "stores", "i(**i)", ms_stores, this)) {
        if (res != m3Err_functionLookupFailed) {
            return Error::from_string_literal(res);
        }
    }

    if (c_string res = m3_LinkRawFunctionEx(mod, "ms", "fetch", "i(*i*i)", ms_fetch, this)) {
        if (res != m3Err_functionLookupFailed) {
            return Error::from_string_literal(res);
        }
    }

    if (c_string res = m3_LinkRawFunctionEx(mod, "ms", "heap_base", "*()", ms_heap_base, this)) {
        if (res != m3Err_functionLookupFailed) {
            return Error::from_string_literal(res);
        }
    }

    if (c_string res = m3_LinkRawFunctionEx(mod, "ms", "heap_end", "*()", ms_heap_end, this)) {
        if (res != m3Err_functionLookupFailed) {
            return Error::from_string_literal(res);
        }
    }

    if (c_string res = m3_CompileModule(mod)) {
        return Error::from_string_literal(res);
    }

    if (!func.init) {
        if (c_string res = m3_FindFunction(&func.init, runtime, "ms_plugin_init")) {
            return Error::from_string_literal(res);
        }
        if (!function_has_signature(func.init, "(i)")) {
            return Error::from_string_literal("expected ms_plugin_init to have signature void(u32)");
        }
    }

    if (!func.process_f32) {
        if (c_string res = m3_FindFunction(&func.process_f32, runtime, "ms_plugin_process_f32")) {
            if (res != m3Err_functionLookupFailed) {
                return Error::from_string_literal(res);
            }
        }
        if (func.process_f32) {
            if (!function_has_signature(func.process_f32, "(**ii)")) {
                return Error::from_string_literal("expected ms_plugin_process_f32 to have signature void(f32* out, f32 const* in, u32 frames, u32 channels)");
            }
        }
    }

    if (!func.process_f64) {
        if (c_string res = m3_FindFunction(&func.process_f64, runtime, "ms_plugin_process_f64")) {
            if (res != m3Err_functionLookupFailed) {
                return Error::from_string_literal(res);
            }
        }
        if (func.process_f64) {
            if (!function_has_signature(func.process_f64, "(**ii)")) {
                return Error::from_string_literal("expected ms_plugin_process_f64 to have signature void(f64* out, f64 const* in, u32 frames, u32 channels)");
            }
        }
    }

    if (!func.process_f32 && !func.process_f64) {
        return Error::from_string_literal("either ms_plugin_process_f32 or ms_plugin_process_f64 needs to be defined");
    }

    if (!func.parameter_count) {
        if (c_string res = m3_FindFunction(&func.parameter_count, runtime, "ms_plugin_parameter_count")) {
            if (res != m3Err_functionLookupFailed) {
                return Error::from_string_literal(res);
            }
        }
        if (func.parameter_count) {
            if (!function_has_signature(func.parameter_count, "i()")) {
                return Error::from_string_literal("expected ms_plugin_parameter_count to have signature u32(void)");
            }
        }
    }

    if (!func.get_parameter) {
        if (c_string res = m3_FindFunction(&func.get_parameter, runtime, "ms_plugin_get_parameter")) {
            if (res != m3Err_functionLookupFailed) {
                return Error::from_string_literal(res);
            }
        }
        if (func.get_parameter) {
            if (!function_has_signature(func.get_parameter, "F(i)")) {
                return Error::from_string_literal("expected ms_plugin_get_parameter to have signature f64(u32 id)");
            }
        }
    }

    if (!func.set_parameter) {
        if (c_string res = m3_FindFunction(&func.set_parameter, runtime, "ms_plugin_set_parameter")) {
            if (res != m3Err_functionLookupFailed) {
                return Error::from_string_literal(res);
            }
        }
        if (func.set_parameter) {
            if (!function_has_signature(func.set_parameter, "(iF)")) {
                return Error::from_string_literal("expected ms_plugin_set_parameter to have signature void(u32 id, f64 value)");
            }
        }
    }

    if (!func.parameter_kind) {
        if (c_string res = m3_FindFunction(&func.parameter_kind, runtime, "ms_plugin_parameter_kind")) {
            if (res != m3Err_functionLookupFailed) {
                return Error::from_string_literal(res);
            }
        }
        if (func.parameter_kind) {
            if (!function_has_signature(func.parameter_kind, "*(i)")) {
                return Error::from_string_literal("expected ms_plugin_parameter_kind to have signature c_string(u32 id)");
            }
        }
    }

    if (!func.parameter_name) {
        if (c_string res = m3_FindFunction(&func.parameter_name, runtime, "ms_plugin_parameter_name")) {
            if (res != m3Err_functionLookupFailed) {
                return Error::from_string_literal(res);
            }
        }
        if (func.parameter_name) {
            if (!function_has_signature(func.parameter_name, "*(i*)")) {
                return Error::from_string_literal("expected ms_plugin_parameter_name to have signature char const*(u32 id, u32* size)");
            }
        }
    }

    if (!func.parameter_min_value) {
        if (c_string res = m3_FindFunction(&func.parameter_min_value, runtime, "ms_plugin_parameter_min_value")) {
            if (res != m3Err_functionLookupFailed) {
                return Error::from_string_literal(res);
            }
        }
        if (func.parameter_min_value) {
            if (!function_has_signature(func.parameter_min_value, "F(i)")) {
                return Error::from_string_literal("expected ms_plugin_parameter_min_value to have signature f64(u32 id)");
            }
        }
    }

    if (!func.parameter_max_value) {
        if (c_string res = m3_FindFunction(&func.parameter_max_value, runtime, "ms_plugin_parameter_max_value")) {
            if (res != m3Err_functionLookupFailed) {
                return Error::from_string_literal(res);
            }
        }
        if (func.parameter_max_value) {
            if (!function_has_signature(func.parameter_max_value, "F(i)")) {
                return Error::from_string_literal("expected ms_plugin_parameter_max_value to have signature f64(u32 id)");
            }
        }
    }

    if (!func.parameter_step_size) {
        if (c_string res = m3_FindFunction(&func.parameter_step_size, runtime, "ms_plugin_parameter_step_size")) {
            if (res != m3Err_functionLookupFailed) {
                return Error::from_string_literal(res);
            }
        }
        if (func.parameter_max_value) {
            if (!function_has_signature(func.parameter_step_size, "F(i)")) {
                return Error::from_string_literal("expected ms_plugin_parameter_step_size to have signature f64(u32 id)");
            }
        }
    }

    if (!func.parameter_option_name) {
        if (c_string res = m3_FindFunction(&func.parameter_option_name, runtime, "ms_plugin_parameter_option_name")) {
            if (res != m3Err_functionLookupFailed) {
                return Error::from_string_literal(res);
            }
        }
        if (func.parameter_option_name) {
            if (!function_has_signature(func.parameter_option_name, "*(ii*)")) {
                return Error::from_string_literal("expected ms_plugin_parameter_option_name to have signature char const*(u32 parameter_id, u32 option_id, u32* size)");
            }
        }
    }

    auto* heap_base_id = m3_FindGlobal(mod, "__heap_base");
    if (!heap_base_id) {
        return Error::from_string_literal("could not find __heap_base");
    }
    M3TaggedValue heap_base_value;
    if (c_string res = m3_GetGlobal(heap_base_id, &heap_base_value)) {
        return Error::from_string_literal(res);
    }
    switch (heap_base_value.type) {
    case c_m3Type_i32:
        heap_base = heap_base_value.value.i32;
        break;
    case c_m3Type_i64:
        heap_base = heap_base_value.value.i64;
        break;
    case c_m3Type_none:
    case c_m3Type_f32:
    case c_m3Type_f64:
    case c_m3Type_unknown:
        return Error::from_string_literal("expected __heap_base to be a pointer");
    }

    auto* heap_end_id = m3_FindGlobal(mod, "__heap_end");
    if (!heap_end_id) {
        return Error::from_string_literal("could not find __heap_end");
    }
    M3TaggedValue heap_end_value;
    if (c_string res = m3_GetGlobal(heap_end_id, &heap_end_value)) {
        return Error::from_string_literal(res);
    }
    switch (heap_end_value.type) {
    case c_m3Type_i32:
        heap_end = heap_end_value.value.i32;
        break;
    case c_m3Type_i64:
        heap_end = heap_end_value.value.i64;
        break;
    case c_m3Type_none:
    case c_m3Type_f32:
    case c_m3Type_f64:
    case c_m3Type_unknown:
        return Error::from_string_literal("expected __heap_end to be a pointer");
    }

    return {};
}

StringView WASMPlugin::name() const
{
    auto* plugin_name_id = m3_FindGlobal(mod, "ms_plugin_name");
    if (!plugin_name_id) {
        return fallback_name.view();
    }
    M3TaggedValue value;
    if (m3_GetGlobal(plugin_name_id, &value)) {
        return fallback_name.view();
    }
    usize offset = 0;
    switch (value.type) {
    case c_m3Type_i32:
        offset = value.value.i32;
        break;
    case c_m3Type_i64:
        offset = value.value.i64;
        break;
    case c_m3Type_none:
    case c_m3Type_f32:
    case c_m3Type_f64:
    case c_m3Type_unknown:
        return fallback_name.view();
    }
    u32 memory_size = 0;
    char* memory = (char*)m3_GetMemory(runtime, &memory_size, 0);
    if (offset > memory_size) {
        return fallback_name.view();
    }
    return StringView::from_c_string_with_max_size(&memory[offset], memory_size - offset);
}

ErrorOr<void> WASMPlugin::init() const
{
    if (c_string res = m3_CallV(func.init, 1, 0ULL)) {
        return Error::from_string_literal(res);
    }

    return {};
}

ErrorOr<void> WASMPlugin::deinit() const
{
    if (!func.deinit) {
        return {};
    }

    if (c_string res = m3_Call(func.deinit, 0, 0)) {
        return Error::from_string_literal(res);
    }

    return {};
}

ErrorOr<void> WASMPlugin::process_f32(f32* out, f32 const* in, u32 frames, u32 channels) const
{
    ASSERT(can_process_f32());
    usize sample_bytes = ((usize)channels) * (usize)frames * sizeof(f32);
    auto wasm_in = TRY(wasm_alloc(sample_bytes).or_error(Error::from_string_literal("could not allocate input buffer")));
    Defer free_wasm_in = [&] {
        wasm_free(sample_bytes);
    };
    auto wasm_out = TRY(wasm_alloc(sample_bytes).or_error(Error::from_string_literal("could not allocate output buffer")));
    Defer free_wasm_out = [&] {
        wasm_free(sample_bytes);
    };
    if (in) {
        TRY(wasm_memcpy(wasm_in, in, sample_bytes));
    } else {
        TRY(wasm_memset(wasm_in, 0, sample_bytes));
    }
    if (c_string res = m3_CallV(func.process_f32, wasm_out, wasm_in, frames, channels)) {
        return Error::from_string_literal(res);
    }
    TRY(wasm_memcpy(out, wasm_out, sample_bytes));
    return {};
}

ErrorOr<void> WASMPlugin::process_f64(f64* out, f64 const* in, u32 frames, u32 channels) const
{
    ASSERT(can_process_f64());
    usize sample_bytes = ((usize)channels) * (usize)frames * sizeof(f64);
    auto wasm_in = TRY(wasm_alloc(sample_bytes).or_error(Error::from_string_literal("could not allocate input buffer")));
    Defer free_wasm_in = [&] {
        wasm_free(sample_bytes);
    };
    auto wasm_out = TRY(wasm_alloc(sample_bytes).or_error(Error::from_string_literal("could not allocate output buffer")));
    Defer free_wasm_out = [&] {
        wasm_free(sample_bytes);
    };
    if (in) {
        TRY(wasm_memcpy(wasm_in, in, sample_bytes));
    } else {
        TRY(wasm_memset(wasm_in, 0, sample_bytes));
    }
    if (c_string res = m3_CallV(func.process_f64, wasm_out, wasm_in, frames, channels)) {
        return Error::from_string_literal(res);
    }
    TRY(wasm_memcpy(out, wasm_out, sample_bytes));
    return {};
}

bool WASMPlugin::can_process_f32() const
{
    return func.process_f32 != nullptr;
}

bool WASMPlugin::can_process_f64() const
{
    return func.process_f64 != nullptr;
}


ErrorOr<u32> WASMPlugin::parameter_count() const
{
    ASSERT(func.parameter_count);
    u32 result = 0;
    if (c_string res = m3_CallV(func.parameter_count)) {
        return Error::from_string_literal(res);
    }
    if (c_string res = m3_GetResultsV(func.parameter_count, &result)) {
        return Error::from_string_literal(res);
    }
    return result;
}

ErrorOr<f64> WASMPlugin::parameter(u32 id) const
{
    ASSERT(func.get_parameter);
    f64 result = 0;
    if (c_string res = m3_CallV(func.get_parameter, id)) {
        return Error::from_string_literal(res);
    }
    if (c_string res = m3_GetResultsV(func.get_parameter, &result)) {
        return Error::from_string_literal(res);
    }
    return result;
}

ErrorOr<void> WASMPlugin::set_parameter(u32 id, f64 value) const
{
    ASSERT(func.set_parameter);
    c_string res = m3_CallV(func.set_parameter, id, value);
    if (res) {
        return Error::from_string_literal(res);
    }
    return {};
}

ErrorOr<StringView> WASMPlugin::parameter_kind(u32 id) const
{
    ASSERT(func.parameter_kind);
    u64 kind = 0;
    if (c_string res = m3_CallV(func.parameter_kind, id)) {
        return Error::from_string_literal(res);
    }
    if (c_string res = m3_GetResultsV(func.parameter_kind, &kind)) {
        return Error::from_string_literal(res);
    }
    if (kind == 0) {
        return ""sv;
    }
    return TRY(wasm_resolve_c_string(kind).or_error(Error::from_string_literal("could not resolve wasm pointer")));
}

ErrorOr<StringView> WASMPlugin::parameter_name(u32 id) const
{
    ASSERT(func.parameter_name);
    u64 name = 0;
    u64 size_ptr = TRY(wasm_alloc(sizeof(u32)).or_error(Error::from_errno(ENOMEM)));
    Defer free_size_ptr = [this] {
        wasm_free(sizeof(u32));
    };
    u32* size = (u32*)wasm_resolve_ptr(size_ptr).unwrap();
    *size = 0;
    if (c_string res = m3_CallV(func.parameter_name, id, size_ptr)) {
        return Error::from_string_literal(res);
    }
    if (c_string res = m3_GetResultsV(func.parameter_name, &name)) {
        return Error::from_string_literal(res);
    }
    if (name == 0) {
        return ""sv;
    }
    return TRY(wasm_resolve_string(name, *size).or_error(Error::from_string_literal("could not resolve wasm pointer")));
}

ErrorOr<f64> WASMPlugin::parameter_min_value(u32 id) const
{
    ASSERT(func.parameter_min_value);
    f64 result = 0;
    if (c_string res = m3_CallV(func.parameter_min_value, id)) {
        return Error::from_string_literal(res);
    }
    if (c_string res = m3_GetResultsV(func.parameter_min_value, &result)) {
        return Error::from_string_literal(res);
    }
    return result;
}

ErrorOr<f64> WASMPlugin::parameter_max_value(u32 id) const
{
    ASSERT(func.parameter_max_value);
    f64 result = 0;
    if (c_string res = m3_CallV(func.parameter_max_value, id)) {
        return Error::from_string_literal(res);
    }
    if (c_string res = m3_GetResultsV(func.parameter_max_value, &result)) {
        return Error::from_string_literal(res);
    }
    return result;
}

ErrorOr<f64> WASMPlugin::parameter_step_size(u32 id) const
{
    ASSERT(func.parameter_step_size);
    f64 result = 0;
    if (c_string res = m3_CallV(func.parameter_step_size, id)) {
        return Error::from_string_literal(res);
    }
    if (c_string res = m3_GetResultsV(func.parameter_step_size, &result)) {
        return Error::from_string_literal(res);
    }
    return result;
}

ErrorOr<StringView> WASMPlugin::parameter_option_name(u32 id, u32 option) const
{
    ASSERT(func.parameter_option_name);
    u64 name = 0;
    u64 size_ptr = TRY(wasm_alloc(sizeof(u32)).or_error(Error::from_errno(ENOMEM)));
    Defer free_size_ptr = [this] {
        wasm_free(sizeof(u32));
    };
    u32* size = (u32*)wasm_resolve_ptr(size_ptr).unwrap();
    *size = 0;
    if (c_string res = m3_CallV(func.parameter_option_name, id, option, size_ptr)) {
        return Error::from_string_literal(res);
    }
    if (c_string res = m3_GetResultsV(func.parameter_option_name, &name)) {
        return Error::from_string_literal(res);
    }
    if (name == 0) {
        return ""sv;
    }
    return TRY(wasm_resolve_string(name, *size).or_error(Error::from_string_literal("could not resolve wasm pointer")));
}


Optional<u64> WASMPlugin::wasm_alloc(u64 size) const
{
    if (heap_end == heap_base) {
        return {};
    }
    if (heap_end - size <= heap_base) {
        return {};
    }
    heap_end -= size;
    return heap_end;
}

void WASMPlugin::wasm_free(u64 size) const
{
    usize memory_size = m3_GetMemorySize(runtime);
    VERIFY(heap_end + size <= memory_size);
    heap_end += size;
}

ErrorOr<u64> WASMPlugin::wasm_memset(u64 wasm_dest, int c, usize size) const
{
    void* dest = TRY(wasm_resolve_ptr(wasm_dest).or_error(Error::from_string_literal("could not resolve wasm ptr")));
    usize memory_size = m3_GetMemorySize(runtime);
    ASSERT(wasm_dest + size <= memory_size);
    memset(dest, c, size);
    return wasm_dest;
}

ErrorOr<u64> WASMPlugin::wasm_memcpy(u64 wasm_dest, void const* from, usize size) const
{
    void* dest = TRY(wasm_resolve_ptr(wasm_dest).or_error(Error::from_string_literal("could not resolve wasm ptr")));
    usize memory_size = m3_GetMemorySize(runtime);
    ASSERT(wasm_dest + size <= memory_size);
    memcpy(dest, from, size);
    return wasm_dest;
}

ErrorOr<void*> WASMPlugin::wasm_memcpy(void* dest, u64 wasm_from, usize size) const
{
    void* from = TRY(wasm_resolve_ptr(wasm_from).or_error(Error::from_string_literal("could not resolve wasm ptr")));
    usize memory_size = m3_GetMemorySize(runtime);
    ASSERT(wasm_from + size <= memory_size);
    memcpy(dest, from, size);
    return dest;
}

Optional<void*> WASMPlugin::wasm_resolve_ptr(u64 ptr) const
{
    u32 memory_size = 0;
    u8* memory = (u8*)m3_GetMemory(runtime, &memory_size, 0);
    if (ptr >= memory_size) {
        return {};
    }
    return memory + ptr;
}

Optional<StringView> WASMPlugin::wasm_resolve_c_string(u64 wasm_ptr) const
{
    auto ptr = wasm_resolve_ptr(wasm_ptr);
    if (!ptr) {
        return {};
    }
    auto memory_size = m3_GetMemorySize(runtime);
    return StringView::from_c_string_with_max_size((c_string)ptr.value(), memory_size - wasm_ptr);
}

Optional<StringView> WASMPlugin::wasm_resolve_string(u64 wasm_ptr, u64 size) const
{
    auto ptr = wasm_resolve_ptr(wasm_ptr);
    if (!ptr) {
        return {};
    }
    auto memory_size = m3_GetMemorySize(runtime);
    if (wasm_ptr + size > memory_size) {
        size = memory_size - wasm_ptr;
    }
    return StringView::from_parts((char*)ptr.value(), size);
}


enum class LogSeverity {
    Info,
    Debug,
    Error,
    Fatal,
};

static StringView severity_string(LogSeverity severity)
{
    switch (severity) {
    case LogSeverity::Info: return "INFO";
    case LogSeverity::Debug: return "DEBUG";
    case LogSeverity::Error: return "ERROR";
    case LogSeverity::Fatal: return "FATAL";
    }
}

static const void* log(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory, LogSeverity severity)
{
    (void)runtime;
    (void)memory;
    auto* plugin = (WASMPlugin*)context->userdata;
    u64 wasm_ptr = *stack;
    auto format = plugin->wasm_resolve_c_string(wasm_ptr);
    if (!format) {
        return m3Err_trapOutOfBoundsMemoryAccess;
    }
    dprintln("{}[{}]: {}", severity_string(severity), plugin->name(), format);
    if (severity == LogSeverity::Fatal) {
        return m3Err_trapAbort;
    }
    return m3Err_none;
}

static const void* ms_info(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory)
{
    return log(runtime, context, stack, memory, LogSeverity::Info);
}

static const void* ms_debug(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory)
{
    return log(runtime, context, stack, memory, LogSeverity::Debug);
}

static const void* ms_error(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory)
{
    return log(runtime, context, stack, memory, LogSeverity::Error);
}

static const void* ms_fatal(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory)
{
    return log(runtime, context, stack, memory, LogSeverity::Fatal);
}

static const void* ms_time_f32(IM3Runtime, IM3ImportContext, uint64_t* stack, void*)
{
    *((f64*)stack) = Core::time();
    return m3Err_none;
}

static const void* ms_time_f64(IM3Runtime, IM3ImportContext, uint64_t* stack, void*)
{
    *((f32*)stack) = (f32)Core::time();
    return m3Err_none;
}

static const void* ms_get_sample_rate(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory)
{
    (void)runtime;
    (void)memory;
    auto* plugin = ((WASMPlugin*)context->userdata);
    *stack = plugin->manager->sample_rate();
    return m3Err_none;
}

static const void* ms_get_channels(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory)
{
    (void)runtime;
    (void)memory;
    auto* plugin = ((WASMPlugin*)context->userdata);
    *stack = plugin->manager->channel_count();
    return m3Err_none;
}

static const void* ms_store(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory)
{
    (void)runtime;
    (void)context;
    (void)stack;
    (void)memory;
    UNIMPLEMENTED();
}

static const void* ms_stores(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory)
{
    (void)runtime;
    (void)context;
    (void)stack;
    (void)memory;
    UNIMPLEMENTED();
}

static const void* ms_fetch(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory)
{
    (void)runtime;
    (void)context;
    (void)stack;
    (void)memory;
    UNIMPLEMENTED();
}

static const void* ms_heap_base(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory)
{
    (void)runtime;
    (void)memory;
    auto* plugin = ((WASMPlugin*)context->userdata);
    *stack = plugin->heap_base;
    return m3Err_none;
}

static const void* ms_heap_end(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory)
{
    (void)runtime;
    (void)memory;
    auto* plugin = ((WASMPlugin*)context->userdata);
    *stack = plugin->heap_end;
    return m3Err_none;
}

static M3ValueType type_char(char c)
{
    switch (c) {
    case 'i': return c_m3Type_i32;
    case 'I': return c_m3Type_i64;
    case 'f': return c_m3Type_f32;
    case 'F': return c_m3Type_f64;
    case '*': return c_m3Type_i32; // FIXME: Assuming wasm32
    }
    VERIFY(false && "unknown type_char");
}

static bool function_has_signature(IM3Function func, StringView signature)
{
    auto return_types = signature.find_first('(').unwrap();
    if (m3_GetRetCount(func) != return_types) {
        return false;
    }
    for (u32 i = 0; i < return_types; i++) {
        auto type = type_char(signature[i]);
        if (m3_GetRetType(func, i) != type) {
            return false;
        }
    }
    auto params = signature.chop_left(return_types + "("sv.size()).shrink(")"sv.size());
    if (m3_GetArgCount(func) != params.size()) {
        return false;
    }
    for (u32 i = 0; i < params.size(); i++) {
        auto type = type_char(params[i]);
        if (m3_GetArgType(func, i) != type) {
            return false;
        }
    }
    return true;
}

}
