#pragma once
#include "./Forward.h"

#include <Rexim/File.h>

struct SDL_Window;
struct FileBrowser {
    Files files;
    usize cursor;
    String_Builder dir_path;
    String_Builder file_path;
};

Errno fb_open_dir(FileBrowser* fb, c_string dir_path);
Errno fb_change_dir(FileBrowser* fb);
void fb_render(FileBrowser const* fb, SDL_Window* window, FreeGlyphAtlas* atlas, SimpleRenderer* sr);
c_string fb_file_path(FileBrowser* fb);
