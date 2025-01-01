#pragma once
#include <SoundIo/SoundIo.h>
#include <Ty/Optional.h>

namespace AU {

struct SoundWriter {
    void (*writer)(void* ptr, f64 value);
    SoundIoFormat format;
};

Optional<SoundWriter> select_writer_for_device(SoundIoDevice*);

}
