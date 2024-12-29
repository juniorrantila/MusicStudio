#include <MS/Universal/Log.h>
#include <MS/Universal/Math.h>
#include <MS/Universal/PlugInfo.h>

char const ms_plugin_name[] = "Hello, World";

static float time;
void ms_process_f32(float* out, float*, unsigned frames);
void ms_process_f32(float* out, float* in, unsigned frames)
{
    (void)in;
    ms_debug("process", "processing %u frames", frames);
    unsigned channels = ms_get_channels();
    float dt = ms_get_sample_rate_f32() / (float)frames;
    for (unsigned i = 0; i < frames; i++, time += dt) {
        for (unsigned c = 0; c < channels; c++) {
            out[i * channels + c] = ms_sin_turns_f32(time * 440.0f);
        }
    }
}

void ms_main(void)
{
    ms_info("main", "Hello");
}
