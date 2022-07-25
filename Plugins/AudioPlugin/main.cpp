#include "AudioPlugin.h"
#include <Vst/Vst.h>

#if _WIN32
#    define export __declspec(dllexport)
#else
#    define export
#endif

export extern "C" Vst::Effect* VSTPluginMain(Vst::HostCallback host)
{
    auto audio_plugin = new AudioPlugin();
    auto effect = new Vst::Effect(audio_plugin);
    audio_plugin->set_host({effect, host});

    return effect;
}
