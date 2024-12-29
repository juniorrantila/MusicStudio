#include "./AudioPlugin.h"

VST_EXPORT extern "C" Vst::Effect* VSTPluginMain(Vst::HostCallback host)
{
    auto audio_plugin = new AudioPlugin();
    auto effect = new Vst::Effect(audio_plugin);
    audio_plugin->set_host({effect, host});
    return effect;
}
