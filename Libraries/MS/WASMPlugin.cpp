#include "./WASMPlugin.h"

#include <Core/Print.h>
#include <Ty/Defer.h>

namespace MS {

static const void* info_puts(IM3Runtime runtime, IM3ImportContext context, uint64_t* stack, void* memory)
{
    auto plugin_name = ((WASMPlugin*)(context->userdata))->name.view();
    uint64_t ptr = *stack;
    uint64_t size = m3_GetMemorySize(runtime);
    if (ptr >= size) {
        return m3Err_trapOutOfBoundsMemoryAccess;
    }
    auto message = StringView::from_c_string(&((char*)memory)[ptr]);
    dprintln("INFO[{}]: {}", plugin_name, message);
    return m3Err_none;
}

WASMPlugin::WASMPlugin(Core::MappedFile&& file, StringBuffer&& name, IM3Environment env, IM3Runtime runtime, IM3Module mod)
    : file(move(file))
    , name(move(name))
    , env(env)
    , runtime(runtime)
    , mod(mod)
{
}

WASMPlugin::WASMPlugin(WASMPlugin&& other)
    : file(move(other.file))
    , name(move(other.name))
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
    name = move(other.name);
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
    if (auto res = m3_ParseModule(env, &mod, file.data(), file.size())) {
        return Error::from_string_literal(res);
    }

    if (auto res = m3_LoadModule(runtime, mod)) {
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
    if (auto res = m3_LinkRawFunctionEx(mod, "ms", "info_puts", "v(*)", info_puts, this)) {
        if (res != m3Err_functionLookupFailed) {
            return Error::from_string_literal(res);
        }
    }

    if (auto res = m3_CompileModule(mod)) {
        return Error::from_string_literal(res);
    }

    if (!main) {
        if (auto res = m3_FindFunction(&main, runtime, "ms_main")) {
            return Error::from_string_literal(res);
        }
        if (m3_GetRetCount(main) != 0) {
            return Error::from_string_literal("expected ms_main to have signature void(void)");
        }
        if (m3_GetArgCount(main) != 0) {
            return Error::from_string_literal("expected ms_main to have signature void(void)");
        }
    }

    return {};
}

ErrorOr<void> WASMPlugin::run()
{
    if (auto res = m3_Call(main, 0, 0)) {
        return Error::from_string_literal(res);
    }

    return {};
}


}
