#pragma once

#include "gfx/Gfx.hpp"

namespace Text {

class Text {
public:
    Gfx::Dimensions d;
    int baseline_y;
    const int pt_size;

    void Render(int x, int baseline_y);
    void Change(const std::string& text) {
        this->text = text;
        this->Update();
    }

    Text(std::string text, int size = 48);

    std::string text;

private:
    void Update();
};

bool Init();
void Quit();

} //namespace Text
