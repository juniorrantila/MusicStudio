#include "./WASMPlugin.h"

#include <WASM3/wasm3.h>
#include <Core/Print.h>
#include <Ty/Defer.h>

namespace MS {

static const void* log_info(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory);
static const void* log_debug(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory);
static const void* log_error(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory);
static const void* log_fatal(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory);

WASMPlugin::WASMPlugin(Core::MappedFile&& file, StringBuffer&& name, IM3Environment env, IM3Runtime runtime, IM3Module mod)
    : file(move(file))
    , fallback_name(move(name))
    , env(env)
    , runtime(runtime)
    , mod(mod)
{
}

WASMPlugin::WASMPlugin(WASMPlugin&& other)
    : file(move(other.file))
    , fallback_name(move(other.fallback_name))
    , env(other.env)
    , runtime(other.runtime)
    , mod(other.mod)
    , main(other.main)
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
    main = other.main;
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

ErrorOr<WASMPlugin> WASMPlugin::create(StringView path)
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
        move(file),
        move(name),
        env,
        runtime,
        mod,
    };
}

ErrorOr<void> WASMPlugin::link()
{
    if (c_string res = m3_LinkRawFunctionEx(mod, "ms", "log_info", "v(**)", log_info, this)) {
        if (res != m3Err_functionLookupFailed) {
            return Error::from_string_literal(res);
        }
    }
    if (c_string res = m3_LinkRawFunctionEx(mod, "ms", "log_debug", "v(**)", log_debug, this)) {
        if (res != m3Err_functionLookupFailed) {
            return Error::from_string_literal(res);
        }
    }
    if (c_string res = m3_LinkRawFunctionEx(mod, "ms", "log_error", "v(**)", log_error, this)) {
        if (res != m3Err_functionLookupFailed) {
            return Error::from_string_literal(res);
        }
    }
    if (c_string res = m3_LinkRawFunctionEx(mod, "ms", "log_fatal", "v(**)", log_fatal, this)) {
        if (res != m3Err_functionLookupFailed) {
            return Error::from_string_literal(res);
        }
    }


    if (c_string res = m3_CompileModule(mod)) {
        return Error::from_string_literal(res);
    }

    if (!main) {
        if (c_string res = m3_FindFunction(&main, runtime, "ms_plugin_main")) {
            return Error::from_string_literal(res);
        }
        if (m3_GetRetCount(main) != 0) {
            return Error::from_string_literal("expected ms_plugin_main to have signature void(void)");
        }
        if (m3_GetArgCount(main) != 0) {
            return Error::from_string_literal("expected ms_plugin_main to have signature void(void)");
        }
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
    return StringView::from_c_string(&memory[offset]);
}

ErrorOr<void> WASMPlugin::run() const
{
    if (c_string res = m3_Call(main, 0, 0)) {
        return Error::from_string_literal(res);
    }

    return {};
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
    auto plugin_name = ((WASMPlugin*)(context->userdata))->name();
    uint64_t ptr = *stack;
    uint64_t size = m3_GetMemorySize(runtime);
    if (ptr >= size) {
        return m3Err_trapOutOfBoundsMemoryAccess;
    }
    auto message = StringView::from_c_string(&((char*)memory)[ptr]);
    dprintln("{}[{}]: {}", severity_string(severity), plugin_name, message);
    if (severity == LogSeverity::Fatal) {
        return m3Err_trapAbort;
    }
    return m3Err_none;
}

static const void* log_info(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory)
{
    return log(runtime, context, stack, memory, LogSeverity::Info);
}

static const void* log_debug(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory)
{
    return log(runtime, context, stack, memory, LogSeverity::Debug);
}

static const void* log_error(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory)
{
    return log(runtime, context, stack, memory, LogSeverity::Error);
}

static const void* log_fatal(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory)
{
    return log(runtime, context, stack, memory, LogSeverity::Fatal);
}

}
