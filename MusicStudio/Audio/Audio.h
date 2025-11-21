#pragma once
#include <Basic/Types.h>
#include <LibCore/Forward.h>

typedef struct StableAudio StableAudio;
typedef struct PersistedState PersistedState;
typedef union TransAudio TransAudio;
typedef struct AudioActor {
    void(*_Atomic const audio_frame)(PersistedState const*, StableAudio*, TransAudio*, f64* const*, u32 frame_count, u32 channel_count);
} AudioActor;

C_API [[nodiscard]] bool audio_actor_init(Actor* actor, FSVolume*, bool use_auto_reload);
C_API void audio_frame();
