#include "BareMinimum.h"
#include <Vst/Vst.h>
#include <stdio.h>

#if _WIN32
#    define export __declspec(dllexport)
#else
#    define export
#endif

export extern "C" Vst::Effect* VSTPluginMain(Vst::HostCallback host)
{
    (void)host;
    auto audio_plugin = new BareMinimum();
    auto effect = new Vst::Effect(audio_plugin);
    return effect;
}
