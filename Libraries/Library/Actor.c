#include "./Actor.h"

#include "./Library.h"

#include <Ty/StringSlice.h>

C_API bool actor_create(Allocator* gpa, void* (*dispatch)(HotReloadEvent), Actor* out)
{
    Actor actor = (Actor){
        .hotreload = (HotReload) {
            .dispatch = dispatch,
        },
        .gpa = gpa,
        .kind = ActorKind_Static,
        .library = 0,
        .model_size = 0,
        .model_align = 0,
        .model = 0,
    };
    usize size = actor_size(&actor);
    usize align = actor_align(&actor);
    if (!size) return *out = actor, true;
    void* state = memalloc(gpa, size, align);
    if (!state) return false;

    actor.model_size = size;
    actor.model_align = align;
    actor.model = state;

    return *out = actor, true;
}

C_API bool actor_create_from_library(Allocator* gpa, Library* library, Actor* out)
{
    Actor actor = (Actor){
        .hotreload = library_hotreload(library),
        .gpa = gpa,
        .kind = ActorKind_Library,
        .library = library,
        .model_size = library_hotreload_state_size(library),
        .model_align = library_hotreload_state_align(library),
        .model = library_hotreload_state(library),
    };
    return *out = actor, true;
}

C_API void actor_destroy(Actor* actor)
{
    switch (actor->kind) {
    case ActorKind_Library:
        library_destroy(actor->library);
        break;
    case ActorKind_Static:
        if (actor->model) memfree(actor->gpa, actor->model, actor->model_size, actor->model_align);
        break;
    }
    memset_canary(actor, sizeof(*actor));
}

C_API void actor_update(Actor* actor)
{
    if (actor->kind != ActorKind_Library) return;
    library_update(actor->library);
}

C_API bool actor_needs_reload(Actor const* actor)
{
    if (actor->kind != ActorKind_Library) return false;
    return library_needs_reload(actor->library);
}

C_API bool actor_reload(Actor* actor)
{
    if (actor->kind != ActorKind_Library) return true;
    if (!library_reload(actor->library)) return false;
    actor->hotreload = library_hotreload(actor->library);
    actor->model = library_hotreload_state(actor->library);
    actor->model_size = library_hotreload_state_size(actor->library);
    actor->model_align = library_hotreload_state_align(actor->library);
    return true;
}

C_API usize actor_size(Actor const* actor)
{
    if (actor->model_size) return actor->model_size;
    return hotreload_size(actor->hotreload);
}

C_API usize actor_align(Actor const* actor)
{
    if (actor->model_align) return actor->model_align;
    switch (actor->kind) {
    case ActorKind_Library:
        return library_hotreload_state_align(actor->library);
    case ActorKind_Static:
        return 16; // FIXME
    }
}

C_API void actor_init(Actor const* actor, void const* arg, usize size)
{
    void(*init)(void*, void const*, usize) = actor_find_symbol(actor, string_slice_from_c_string("init"));
    if (!init) return;
    init(actor->model, arg, size);
}

C_API void actor_deinit(Actor const* actor)
{
    void(*deinit)(void*) = actor_find_symbol(actor, string_slice_from_c_string("deinit"));
    if (!deinit) return;
    deinit(actor->model);
}

C_API void* actor_find_symbol(Actor const* actor, StringSlice symbol_name)
{
    return hotreload_find_symbol(actor->hotreload, symbol_name);
}
