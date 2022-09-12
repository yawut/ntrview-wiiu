#pragma once

#include "gfx/Gfx.hpp"

namespace Text {

class Text {
public:
    Gfx::Dimensions d;
    int baseline_y;
    const int pt_size;

    void Render(int x, int baseline_y, Gfx::rgb colour);
    void Change(const std::u16string& text) {
        this->text = text;
        this->Update();
    }

    Text(std::u16string text, int size = 32);
    Text(char16_t c, int size = 32) : Text(std::u16string(&c, 1), size) {};

    std::u16string text;

private:
    void Update();
};

bool Init();
void Quit();

} //namespace Text
