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

    std::u16string text;

private:
    void Update();
};

bool Init();
void Quit();

} //namespace Text
