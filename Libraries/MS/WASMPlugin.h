#pragma once
#include <Core/MappedFile.h>
#include <Ty/StringBuffer.h>
#include <WASM3/wasm3.h>

namespace MS {

struct WASMPlugin {
    Core::MappedFile file;
    StringBuffer name {};
    IM3Environment env { nullptr };
    IM3Runtime runtime { nullptr };
    IM3Module mod { nullptr };
    IM3Function main { nullptr };

    WASMPlugin(WASMPlugin&&);
    WASMPlugin& operator=(WASMPlugin&&);
    ~WASMPlugin();

    static ErrorOr<WASMPlugin> create(StringView path);

    ErrorOr<void> link();
    ErrorOr<void> run();

private:
    WASMPlugin(Core::MappedFile&& file, StringBuffer&& name, IM3Environment env, IM3Runtime runtime, IM3Module mod);
};

}
