#pragma once
#include <Basic/Types.h>
#include <LibCore/Forward.h>

typedef struct StableLayout StableLayout;
typedef union TransLayout TransLayout;
typedef struct UIActor {
    void(* _Atomic const layout_frame)(StableLayout*, TransLayout*);
} UIActor;

C_API bool ui_actor_init(Actor* actor, FSVolume*, bool use_auto_reload);
