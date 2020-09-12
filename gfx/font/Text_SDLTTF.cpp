#include "Text.hpp"
#include "gfx/Gfx.hpp"

//very simple glue class for legacy Gfx::Texture(std::string). Will soon be
//moved in here proper.

namespace Text {

bool Init() { return true; }
void Quit() {}

void Text::Render(int x, int y) {
    tex.Render((Gfx::Rect) {
        .x = x,
        .y = y,
        .d = tex.d,
    });
}

Text::Text(std::string text) :
    text(text), tex(text) {
    this->d = this->tex.d;
}

} //namespace Text
