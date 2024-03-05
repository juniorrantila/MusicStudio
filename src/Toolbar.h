#pragma once
#include <UI/Forward.h>
#include <Rexim/LA.h>
#include "./Widget.h"

struct Toolbar : Widget {
    void render(UI::UI& ui, EventLoop&, Vec4f box);
};
