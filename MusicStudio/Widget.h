#pragma once
#include "./Forward.h"

#include <UI/Forward.h>

#include <Rexim/LA.h>

struct Widget {
    virtual ~Widget() {}
    virtual void render(UI::UI&, EventLoop& event_loop, Vec4f box) = 0;
};
