#include "Library.h"
#include <dlfcn.h>

namespace Core {

ErrorOr<Library> Library::open_local(c_string path)
{
    void* handle = dlopen(path, RTLD_LOCAL);
    if (!handle) {
        return Error::from_string_literal("could not open library");
    }
    return Library(handle);
}

void Library::destroy()
{
    dlclose(m_handle);
}

ErrorOr<void*> Library::fetch_symbol(c_string name) const
{
    void* symbol = dlsym(m_handle, name);
    if (!symbol) {
        return Error::from_string_literal("could not fetch symbol");
    }
    return symbol;
}

}
