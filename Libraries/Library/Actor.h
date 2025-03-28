#pragma once
#include <Ty/Base.h>
#include <Ty2/Allocator.h>
#include <Ty/StringSlice.h>
#include <FS/FSVolume.h>

#include "./Library.h"

typedef enum ActorKind {
    ActorKind_Library,
    ActorKind_Static,
} ActorKind;

typedef struct Actor {
    HotReload hotreload;
    Allocator* gpa;
    Library* library;
    ActorKind kind;
    usize model_size;
    usize model_align;
    void* model;
} Actor;

C_API bool actor_create(Allocator* gpa, void* (*dispatch)(HotReloadEvent), Actor*);
C_API bool actor_create_from_library(Allocator* gpa, Library*, Actor*);
C_API void actor_destroy(Actor*);

C_API void actor_update(Actor*);
C_API bool actor_needs_reload(Actor const*);
C_API bool actor_reload(Actor *);

C_API usize actor_size(Actor const*);
C_API usize actor_align(Actor const*);
C_API void actor_init(Actor const*, void const*, usize);
C_API void actor_deinit(Actor const*);
C_API void* actor_find_symbol(Actor const*, StringSlice);
