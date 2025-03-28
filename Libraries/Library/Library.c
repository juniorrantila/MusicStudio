#include "./Library.h"

#include "./HotReload.h"

#include <FS/FSVolume.h>
#include <Ty/Verify.h>

#include <dlfcn.h>
#include <mach-o/dyld.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    Library_Bundle,
    Library_Shared,
} LibraryKind;

typedef struct HotReloadState {
    Allocator* gpa;
    HotReload dispatch;
    FSVolume* volume;
    FileID file;

    void* state;
    usize size;
    bool needs_reload;
} HotReloadState;

struct Library {
    void* handle;

    HotReloadState hotreload;

    LibraryKind kind;
};

Library* library_create_from_memory(c_string identifier, void* mem, usize size, e_library* error)
{
    Library* lib = (Library*)malloc(sizeof(Library));
    if (!lib) {
        *error = e_library_could_not_allocate;
        return 0;
    }
    memset(lib, 0, sizeof(Library));
    lib->kind = Library_Bundle;

    NSObjectFileImageReturnCode rc;

    NSObjectFileImage img;
    rc = NSCreateObjectFileImageFromMemory(mem, size, &img);
    if (rc != NSObjectFileImageSuccess) {
      if (error) *error = e_library_could_not_create_object;
      free(lib);
      return 0;
    }

    NSModule mod = NSLinkModule(img, identifier, NSLINKMODULE_OPTION_PRIVATE);
    if (!mod) {
        if (error) *error = e_library_could_not_link;
        free(lib);
        return 0;
    }
    NSDestroyObjectFileImage(img);

    lib->handle = mod;
    return lib;
}


Library* library_create_from_path(c_string path, e_library* error)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        if (error) *error = e_library_could_not_open;
        return 0;
    }
    struct stat st;
    if (fstat(fd, &st) < 0) {
        if (error) *error = e_library_could_not_stat;
        close(fd);
        return 0;
    }
    void* mem = mmap(0, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (mem == (void*)-1) {
        if (error) *error = e_library_could_not_map;
        close(fd);
        return 0;
    }

    Library* lib = library_create_from_memory(path, mem, st.st_size, error);
    munmap(mem, st.st_size);
    close(fd);
    return lib;
}

void library_destroy(Library* library)
{
    switch (library->kind) {
    case Library_Bundle:
        NSUnLinkModule(library->handle, NSUNLINKMODULE_OPTION_NONE);
        free(library);
        break;
    case Library_Shared:
        hotreload_deinit(library->hotreload.dispatch, library->hotreload.gpa, library->hotreload.state, library->hotreload.size);
        memfree(library->hotreload.gpa, library->hotreload.state, library->hotreload.size, 16);
        dlclose(library->handle);
        memfree(library->hotreload.gpa, library, sizeof(Library), alignof(Library));
        break;
    }
}

void* library_get_symbol(Library const* library, c_string name)
{
    if (!library->handle) return 0;
    switch (library->kind) {
    case Library_Bundle: {
        NSSymbol symbol = NSLookupSymbolInModule(library->handle, name);
        if (!symbol) return 0;
        return NSAddressOfSymbol(symbol);
    }
    case Library_Shared:
        return dlsym(library->handle, name);
    }
}

static bool hot_reload(Library* lib);

C_API Library* library_hotreloadable(Allocator* gpa, FSVolume* volume, FileID file, e_library* error)
{
    Library* lib = memalloc(gpa, sizeof(Library), alignof(Library));
    if (!lib) {
        *error = e_library_could_not_allocate;
        goto fi_1;
    }
    memset(lib, 0, sizeof(Library));
    lib->kind = Library_Shared;
    lib->hotreload.gpa = gpa;
    lib->hotreload.volume = volume;
    lib->hotreload.file = file;

    if (!hot_reload(lib)) {
        *error = e_library_not_hotreloadable;
        goto fi_2;
    }
    return lib;
fi_2:
    memfree(gpa, lib, sizeof(Library), alignof(Library));
fi_1:
    return 0;
}

static bool hot_reload(Library* lib)
{
    VERIFY(lib->kind == Library_Shared);
    FSFile file = fs_volume_use(lib->hotreload.volume, lib->hotreload.file);
    VERIFY(file.kind == FSFileMount_SystemMount);
    StringSlice path = file.system_mount.path;
    HotReloadState* hot = &lib->hotreload;

    char* cpath = (char*)memclone_zero_extend(hot->gpa, path.items, path.count, 1, 1);
    if (!cpath) goto fi_0;
    void* new_handle = dlopen(cpath, RTLD_LAZY);
    memfree(hot->gpa, cpath, path.count + 1, 1);
    if (!new_handle) goto fi_1;

    usize old_size = hot->size;
    void* old_state = hot->state;
    HotReload old_dispatch = lib->hotreload.dispatch;
    void* old_handle = lib->handle;

    HotReload new_dispatch = {
        .dispatch = dlsym(new_handle, "hotreload_dispatch"),
    };
    if (!new_dispatch.dispatch) goto fi_2;

    usize new_size = hotreload_size(new_dispatch);
    void* new_state = 0;
    if (new_size) {
        new_state = memalloc(lib->hotreload.gpa, new_size, 16);
        if (!new_state) goto fi_3;
        memset(new_state, 0, new_size);
    }

    if (old_dispatch.dispatch) hotreload_deinit(old_dispatch, hot->gpa, old_state, old_size);
    usize smaller = old_size < new_size ? old_size : new_size;
    memcpy(new_state, old_state, smaller);
    hotreload_init(new_dispatch, hot->gpa, new_state, new_size);

    hot->state = new_state;
    hot->size = new_size;
    hot->dispatch = new_dispatch;
    lib->handle = new_handle;
    hot->needs_reload = false;

    if (old_state) memfree(hot->gpa, old_state, old_size, 16);
    if (old_handle) dlclose(old_handle);
    return true;
fi_3:
fi_2:
    dlclose(new_handle);
fi_1:
fi_0:
    return false;
}

C_API void library_update(Library* lib)
{
    VERIFY(lib->kind == Library_Shared);
    FSFile* file = fs_volume_use_ref(lib->hotreload.volume, lib->hotreload.file);
    if (fs_file_needs_reload(file)) {
        lib->hotreload.needs_reload = true;
    }
}

C_API bool library_needs_reload(Library const* lib)
{
    if (lib->kind != Library_Shared) return false;
    return lib->hotreload.needs_reload;
}

C_API bool library_reload(Library* lib)
{
    if (lib->kind != Library_Shared) return true;
    FSFile* file = fs_volume_use_ref(lib->hotreload.volume, lib->hotreload.file);
    if (fs_file_needs_reload(file)) {
        if (!fs_file_reload(file)) {
            return false;
        }
    }

    for (usize retry = 0; retry < 10; retry++) {
        if (hot_reload(lib)) return true;
    }
    return false;
}

C_API void* library_hotreload_state(Library const* lib)
{
    VERIFY(lib->kind == Library_Shared);
    return lib->hotreload.state;
}

C_API usize library_hotreload_state_size(Library const* lib)
{
    VERIFY(lib->kind == Library_Shared);
    return lib->hotreload.size;
}

C_API usize library_hotreload_state_align(Library const* lib)
{
    VERIFY(lib->kind == Library_Shared);
    return 16;
}

C_API HotReload library_hotreload(Library const* lib)
{
    VERIFY(lib->kind == Library_Shared);
    return lib->hotreload.dispatch;
}

C_API usize hotreload_size(HotReload r)
{
    if (!r.dispatch) return 0;
    return (usize)r.dispatch((HotReloadEvent){
        .gpa = 0,
        .size = 0,
        .state = 0,
        .tag = HotReloadTag_Size,
    });
}

C_API void* hotreload_find_symbol(HotReload r, StringSlice symbol)
{
    return r.dispatch((HotReloadEvent){
        .gpa = 0,
        .size = symbol.count,
        .state = (void*)symbol.items,
        .tag = HotReloadTag_Find,
    });
}

c_string library_strerror(e_library error)
{
    switch (error) {
#define LIBRARY_ERROR(ident, pretty, ...) case e_library_##ident: return pretty;
#include "./Error.def"
#undef LIBRARY_ERROR
    }
    return "(unknown error)";
}
