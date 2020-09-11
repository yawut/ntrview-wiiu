#pragma once

#include "gfx/Gfx.hpp"

namespace Text {

class Text {
public:
    Gfx::Dimensions d;
    void Render(int x, int y);

    Text(std::string Text);

    const std::string text;
};

bool Init();
void Quit();

} //namespace Text
