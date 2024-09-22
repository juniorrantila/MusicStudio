#include "BareMinimum.h"
#include <Vst/Vst.h>

Vst::Effect* VSTPluginMain(Vst::HostCallback host)
{
    (void)host;
    return new Vst::Effect(new BareMinimum());
}
