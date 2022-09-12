#pragma once

#include "config/Config.hpp"
#include "gfx/Gfx.hpp"
#include "gfx/font/Text.hpp"
#include "input/Input.hpp"
#include "menu/StatusOverlay.hpp"
#include <turbojpeg.h>

class Menu {
public:
    Menu(Config& config, tjhandle tj_handle);
    bool Update(Config& config, bool open, const Input::WiiUInputState& input);
    void Render();

    StatusOverlay overlay;

private:
    Gfx::Texture bg;
    class MenuItem {
    public:
        Text::Text label;
        Text::Text text;
        int id;
        MenuItem(std::u16string label, int id) :
            label(label), text(u""), id(id) {}
    };
    MenuItem ip;
    MenuItem profile;
    static int DrawMenuItem(MenuItem& item, int y);

    constexpr static Gfx::rgb selected_colour = {
        .r = 255,
        .a = 128,
    };
    constexpr static Gfx::rgb unselected_colour = {};
    constexpr static Gfx::rgb text_colour = (Gfx::rgb) {
        .a = 0xFF
    };
};
