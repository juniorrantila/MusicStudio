#pragma once
#include <SoundIo/SoundIo.h>
#include <Basic/Types.h>

typedef struct AUSoundIoWriter {
    void (*writer)(void* ptr, f64 value);
    SoundIoFormat format;
} AUSoundIoWriter;

C_API [[nodiscard]] bool au_select_soundio_writer_for_device(SoundIoDevice*, AUSoundIoWriter*);
