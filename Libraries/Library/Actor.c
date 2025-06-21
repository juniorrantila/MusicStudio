#include "./Actor.h"

#include <FS/FSVolume.h>
#include <Ty/Verify.h>

#include <string.h>
#include <dlfcn.h>

static bool is_library(Actor const* actor);
static usize size(void*(*dispatch)(ActorEvent));
static usize align(void*(*dispatch)(ActorEvent));
static bool init(void*(*dispatch)(ActorEvent), u8* state, void const* arg, usize arg_size);
static void deinit(void*(*dispatch)(ActorEvent), u8* state);
static void bind(void*(*dispatch)(ActorEvent), u8* state);

C_API bool actor_init(Logger* debug, void* (*dispatch)(ActorEvent), void const* arg, usize arg_size, Actor* out)
{
    VERIFY(dispatch);
    VERIFY(out);

    usize state_size = size(dispatch);
    if (state_size > actor_max_state_size) {
        log_error_if(debug, "state size larger than allowed (size: %zu, max: %zu)", state_size, actor_max_state_size);
        return false;
    }
    usize state_align = align(dispatch);
    if (state_align > actor_max_state_align) {
        log_error_if(debug, "state align larger than allowed (align: %zu, max: %zu)", state_align, actor_max_state_align);
        return false;
    }

    *out = (Actor){
        .current_state = out->state[0],
        .state_index = 0,
        .library = {
            .volume = 0,
            .dispatch_symbol = 0,
            .file = {},
            .needs_reload = false,
            .handles = {},
        },
        .debug = debug,
        .dispatch = dispatch,
        .state = {},
    };
    if (!init(dispatch, out->current_state, arg, arg_size)) {
        log_error_if(debug, "running dispatch init failed");
        memset(out, 0, sizeof(*out));
        return false;
    }
    bind(dispatch, out->current_state);
    return true;
}

C_API bool actor_init_reloadable(Logger* debug, FSVolume* volume, FileID library, c_string dispatch_name, void const* arg, usize arg_size, Actor* out)
{
    VERIFY(volume);
    VERIFY(dispatch_name);
    VERIFY(out);

    FSFile const* file = fs_volume_use_ref(volume, library);
    VERIFY(file->kind == FSFileMount_SystemMount);

    c_string path = file->system_mount.path.items;

    void* handle = dlopen(path, RTLD_NOW);
    if (!handle) {
        log_error_if(debug, "could not open library '%s'", path);
        return false;
    }

    void* (*dispatch)(ActorEvent) = dlsym(handle, dispatch_name);
    if (!dispatch) {
        log_error_if(debug, "could not find '%s'", dispatch_name);
        return false;
    }

    usize state_size = size(dispatch);
    if (state_size > actor_max_state_size) {
        log_error_if(debug, "state size was bigger than allowed (size: %zu, max: %zu)", state_size, actor_max_state_size);
        dlclose(handle);
        return false;
    }
    usize state_align = align(dispatch);
    if (state_align > actor_max_state_align) {
        log_error_if(debug, "state align was bigger than allowed (align: %zu, max: %zu)", state_align, actor_max_state_align);
        dlclose(handle);
        return false;
    }

    *out = (Actor){
        .current_state = out->state[0],
        .state_index = 0,
        .library = {
            .volume = volume,
            .dispatch_symbol = dispatch_name,
            .file = library,
            .needs_reload = false,
            .handles = {
                handle,
            },
        },
        .debug = debug,
        .dispatch = dispatch,
        .state = {},
    };
    memset(out->current_state, 0, state_size);
    if (!init(dispatch, out->current_state, arg, arg_size)) {
        log_error_if(debug, "running dispatch init failed");
        memset(out, 0, sizeof(*out));
        return false;
    }
    bind(dispatch, out->current_state);
    return true;
}

C_API void actor_deinit(Actor* actor)
{
    deinit(actor->dispatch, actor->current_state);
    for (usize i = 0; i < actor_max_state_size; i++) {
        void* handle = actor->library.handles[i];
        if (handle) dlclose(handle);
    }
    memset(actor, 0, sizeof(*actor));
}

C_API void actor_update(Actor* actor)
{
    if (!is_library(actor)) return;
    if (fs_file_needs_reload(fs_volume_use_ref(actor->library.volume, actor->library.file))) {
        actor->library.needs_reload = true;
    }
}

C_API bool actor_needs_reload(Actor const* actor)
{
    if (!is_library(actor)) return false;
    return actor->library.needs_reload;
}

C_API bool actor_reload(Actor* actor)
{
    if (!is_library(actor)) return true;
    if (!actor_needs_reload(actor)) return true;

    FSFile* file = fs_volume_use_ref(actor->library.volume, actor->library.file);
    if (fs_file_needs_reload(file) && !fs_file_reload(file)) {
        log_error_if(actor->debug, "could not reload actor file");
        return false;
    }
    if (file->kind != FSFileMount_SystemMount) return true;
    int path_size = (int)file->system_mount.path.count;
    c_string path = file->system_mount.path.items;
    void* handle = dlopen(path, RTLD_NOW);
    if (!handle) {
        log_error_if(actor->debug, "could not open library '%.*s'", path_size, path);
        return false;
    }

    c_string symbol = actor->library.dispatch_symbol;
    void* (*dispatch)(ActorEvent) = dlsym(handle, symbol);
    if (!dispatch) {
        log_error_if(actor->debug, "actor '%.*s' does not contain symbol '%s'", path_size, path, symbol);
        return false;
    }
    usize new_size = size(dispatch);
    if (new_size > actor_max_state_size) {
        log_error_if(actor->debug, "actor size too big (size: %zu, max: %zu)", new_size, actor_max_state_size);
        return false;
    }
    usize new_align = size(dispatch);
    if (new_align > actor_max_state_align) {
        log_error_if(actor->debug, "actor align too big (size: %zu, max: %zu)", new_align, actor_max_state_align);
        return false;
    }

    usize old_size = size(actor->dispatch);
    usize old_index = actor->state_index;
    usize new_index = (old_index + 1) % actor_max_states;
    if (actor->library.handles[new_index]) dlclose(actor->library.handles[new_index]);
    actor->library.handles[new_index] = handle;
    actor->state_index = new_index;
    actor->dispatch = dispatch;

    u8* new_state = actor->state[new_index];
    u8* old_state = actor->state[old_index];

    memset(new_state, 0, new_size);
    memcpy(new_state, old_state, old_size);
    bind(dispatch, new_state);
    actor->current_state = new_state;
    actor->library.needs_reload = false;

    return true;
}

static usize size(void*(*dispatch)(ActorEvent))
{
    return (usize)dispatch((ActorEvent){
        .self = 0,
        .arg = 0,
        .size = 0,
        .tag = ActorEventTag_Size,
    });
}

static usize align(void*(*dispatch)(ActorEvent))
{
    return (usize)dispatch((ActorEvent){
        .self = 0,
        .arg = 0,
        .size = 0,
        .tag = ActorEventTag_Align,
    });
}

static bool init(void*(dispatch)(ActorEvent), u8* state, void const* arg, usize arg_size)
{
    return (bool)dispatch((ActorEvent){
        .self = state,
        .arg = arg,
        .size = arg_size,
        .tag = ActorEventTag_Init,
    });
}

static void deinit(void*(dispatch)(ActorEvent), u8* state)
{
    dispatch((ActorEvent){
        .self = state,
        .arg = 0,
        .size = 0,
        .tag = ActorEventTag_Deinit,
    });
}

static void bind(void*(*dispatch)(ActorEvent), u8* state)
{
    dispatch((ActorEvent){
        .self = state,
        .arg = 0,
        .size = 0,
        .tag = ActorEventTag_Bind,
    });
}

static bool is_library(Actor const* actor)
{
    return actor->library.volume != 0;
}
