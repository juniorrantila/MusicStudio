#include "./Actor.h"

#include <Ty2/Base.h>
#include <FS/FSVolume.h>
#include <Ty/Verify.h>

#include <string.h>
#include <dlfcn.h>

static bool is_library(Actor const* actor);

C_API Actor actor_init(void (*dispatch)(void))
{
    return (Actor){
        .dispatch = dispatch,
    };
}

C_API bool actor_init_reloadable(Actor* actor, Logger* debug, FSVolume* volume, FileID library, c_string dispatch_name)
{
    VERIFY(actor);
    VERIFY(volume);
    VERIFY(dispatch_name);

    FSFile const* file = fs_volume_use_ref(volume, library);
    VERIFY(file->kind == FSFileMount_SystemMount);

    c_string path = file->system_mount.path.items;

    void* handle = dlopen(path, RTLD_NOW);
    if (!handle) {
        log_error_if(debug, "could not open library '%s'", path);
        return false;
    }

    void(*dispatch)(void) = dlsym(handle, dispatch_name);
    if (!dispatch) {
        log_error_if(debug, "could not find '%s'", dispatch_name);
        return false;
    }

    *actor = (Actor){
        .debug = debug,
        .volume = volume,
        .handles = {
            handle,
        },
        .dispatch_name = dispatch_name,
        .dispatch = dispatch,
        .current_handle = 0,
        .needs_reload = false,
        .library = library,
    };
    return true;
}

C_API void actor_deinit(Actor* actor)
{
    for (usize i = 0; i < ty_array_size(actor->handles); i++) {
        void* handle = actor->handles[i];
        if (handle) dlclose(handle);
    }
    memset(actor, 0, sizeof(*actor));
}

C_API void actor_update(Actor* actor)
{
    if (!is_library(actor)) return;
    if (fs_file_needs_reload(fs_volume_use_ref(actor->volume, actor->library))) {
        actor->needs_reload = true;
    }
}

C_API bool actor_needs_reload(Actor const* actor)
{
    if (!is_library(actor)) return false;
    return actor->needs_reload;
}

C_API bool actor_reload(Actor* actor)
{
    if (!is_library(actor)) return true;
    if (!actor_needs_reload(actor)) return true;

    FSFile* file = fs_volume_use_ref(actor->volume, actor->library);
    if (fs_file_needs_reload(file) && !fs_file_reload(file)) {
        log_error_if(actor->debug, "could not reload actor file");
        return false;
    }
    if (file->kind != FSFileMount_SystemMount) return true;
    int path_size = (int)file->system_mount.path.count;
    c_string path = file->system_mount.path.items;
    void* handle = dlopen(path, RTLD_NOW|RTLD_LOCAL);
    if (!handle) {
        log_error_if(actor->debug, "could not open library '%.*s'", path_size, path);
        return false;
    }

    c_string symbol = actor->dispatch_name;
    void (*dispatch)(void) = dlsym(handle, symbol);
    if (!dispatch) {
        log_error_if(actor->debug, "actor '%.*s' does not contain symbol '%s'", path_size, path, symbol);
        return false;
    }

    u64 old_index = actor->current_handle;
    u64 new_index = (old_index + 1) % ty_array_size(actor->handles);
    if (actor->handles[new_index]) dlclose(actor->handles[new_index]);
    actor->handles[new_index] = handle;
    actor->current_handle = new_index;
    actor->needs_reload = false;

    ty_write_barrier();
    // __builtin___clear_cache((void*)dispatch, ((char*)dispatch) + 4096);
    actor->dispatch = dispatch;

    return true;
}

static bool is_library(Actor const* actor)
{
    return actor->volume != 0;
}
