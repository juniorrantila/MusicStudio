#include "./SoundIo.h"

#include <Ty/Limits.h>

namespace AU {

Optional<SoundWriter> select_writer_for_device(SoundIoDevice* device)
{
    if (soundio_device_supports_format(device, SoundIoFormatFloat64NE)) {
        return SoundWriter{
            .writer = [](void* ptr, f64 sample) {
                f64* buf = (f64*)ptr;
                *buf = sample;
            },
            .format = SoundIoFormatFloat64NE,
        };
    }

    if (soundio_device_supports_format(device, SoundIoFormatFloat32NE)) {
        return SoundWriter{
            .writer = [](void* ptr, f64 sample) {
                f32* buf = (f32*)ptr;
                *buf = (f32)sample;
            },
            .format = SoundIoFormatFloat32NE,
        };
    }

    if (soundio_device_supports_format(device, SoundIoFormatS32NE)) {
        return SoundWriter{
            .writer = [](void* ptr, f64 sample) {
                i32* buf = (i32*)ptr;
                f64 range = (f64)Limits<i32>::max() - (f64)Limits<i32>::min();
                f64 val = sample * range / 2.0;
                *buf = (i32)val;
            },
            .format = SoundIoFormatS32NE,
        };
    }

    if (soundio_device_supports_format(device, SoundIoFormatS16NE)) {
        return SoundWriter{
            .writer = [](void* ptr, f64 sample) {
                i16* buf = (i16*)ptr;
                f64 range = (f64)Limits<i16>::min() - (f64)Limits<i16>::max();
                f64 val = sample * range / 2.0;
                *buf = (i16)val;
            },
            .format = SoundIoFormatS16NE,
        };
    }

    return {};
}

}
