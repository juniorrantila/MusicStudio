#include "./Audio.h"

#include "./WAV.h"

namespace AU {

static ErrorOr<Audio> transcode(WAV wav);

ErrorOr<Audio> Audio::decode(AudioFormat format, Bytes bytes)
{
    switch (format) {
    case AudioFormat::WAV: {
        return TRY(transcode(TRY(WAV::decode(bytes))));
    }
    }
}

static ErrorOr<Audio> transcode(WAV wav)
{
    auto samples = TRY(Buffer<f64>::create(wav.frame_count() * wav.channel_count()));
    TRY(wav.write_into(&samples));
    return Audio(move(samples), AudioSpec{
        .frame_count = wav.frame_count(),
        .channel_count = wav.channel_count(),
        .sample_rate = wav.sample_rate(),
    });
}

}
