#pragma once
#include <Ty/Base.h>

#include <Ty2/Target.h>
#include <FS/FSVolume.h>

#define actor_library_path(name) ty_library_path(name)

typedef enum ActorEventTag {
    ActorEventTag_Size,
    ActorEventTag_Align,
    ActorEventTag_Init,
    ActorEventTag_Deinit,
    ActorEventTag_Bind,
} ActorEventTag;

typedef struct ActorEvent {
    void* self;
    void const* arg;
    usize size;
    ActorEventTag tag;
} ActorEvent;

static constexpr usize actor_max_states = 4;
static constexpr usize actor_max_state_size = 256;
static constexpr usize actor_max_state_align = 256;

typedef struct Actor {
    void* (*dispatch)(ActorEvent);
    usize state_index;
    Logger* debug;
    struct {
        FSVolume* volume;
        c_string dispatch_symbol;

        FileID file;
        bool needs_reload;
        void* handles[actor_max_states];
    } library;

    u8* _Atomic current_state;
    alignas(actor_max_state_align) u8 state[actor_max_states][actor_max_state_size];
} Actor;

C_API bool actor_init(Logger* debug, void* (*dispatch)(ActorEvent), void const* arg, usize arg_size, Actor*);
C_API bool actor_init_reloadable(Logger* debug, FSVolume* volume, FileID library, c_string dispatch_name, void const* arg, usize arg_size, Actor*);
C_API void actor_deinit(Actor*);

C_API void actor_update(Actor*);
C_API bool actor_needs_reload(Actor const*);
C_API bool actor_reload(Actor*);
