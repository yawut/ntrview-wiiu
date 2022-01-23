#pragma once

#include "config/Config.hpp"
#include "gfx/Gfx.hpp"
#include "gfx/font/Text.hpp"
#include "input/Input.hpp"
#include "menu/StatusOverlay.hpp"

class Menu {
public:
    Menu(Config& config);
    bool Update(Config& config, bool open, const Input::WiiUInputState& input);
    void Render();

    StatusOverlay overlay;

private:
    Gfx::FillRect bg;
    class MenuItem {
    public:
        Text::Text label;
        Text::Text text;
        int id;
        MenuItem(std::string label, int id) :
            label(label), text(""), id(id) {}
    };
    MenuItem ip;
    MenuItem profile;
    static int DrawMenuItem(MenuItem& item, int y);

    constexpr static Gfx::rgb selected_colour = {
        .r = 255,
        .a = 128,
    };
    constexpr static Gfx::rgb unselected_colour = {};
};
