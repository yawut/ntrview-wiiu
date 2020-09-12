#pragma once

#include "gfx/Gfx.hpp"

namespace Text {

class Text {
public:
    Gfx::Dimensions d;
    void Render(int x, int y);

    Text(std::string text);

    const std::string text;
#ifdef GFX_SDL
    Gfx::Texture tex;
#endif
};

bool Init();
void Quit();

} //namespace Text
