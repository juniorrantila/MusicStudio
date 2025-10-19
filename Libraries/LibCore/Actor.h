#pragma once
#include <LibTy/Base.h>

#include <Basic/Target.h>

#include "./FSVolume.h"

#define actor_library_path(name) ty_library_path(name)

typedef struct Actor {
    FSVolume* volume;
    void* handles[8];
    c_string dispatch_name;
    void (*_Atomic dispatch)(void);
    u32 current_handle;
    u32 needs_reload;
    FileID library;
} Actor;

C_API Actor actor_init(void (*dispatch)(void));
C_API bool actor_init_reloadable(Actor*, FSVolume* volume, FileID library, c_string dispatch_name);
C_API void actor_deinit(Actor*);

C_API void actor_update(Actor*);
C_API bool actor_needs_reload(Actor const*);
C_API bool actor_reload(Actor*);
