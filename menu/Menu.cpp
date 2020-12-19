#include "Menu.hpp"

#include <nn/swkbd.h>
#include <string>
#include <locale>

enum MenuItemID {
    NONE = 0,
    IP_ADDRESS,
    PROFILE,
    MAX,
};

Menu::Menu(Config& config) :
overlay(config.networkconfig.host),
bg(
    (Gfx::Rect) {
        .x = 0,
        .y = 0,
        .d = {
            .w = 854,
            .h = 480,
        },
    },
    (Gfx::rgb) { .a = 200 }),
ip("3DS IP:", IP_ADDRESS),
profile("Profile:", PROFILE) {
    config.menu_changed = true;
}

const static Gfx::Rect pad = (Gfx::Rect) {
    .x = 30,
    .y = 30,
};

static bool swkbd_open = false;
static int selected_item = NONE;
static bool editing_item = false;

int Menu::DrawMenuItem(MenuItem& item, int y) {
    item.label.Render(pad.x, y + pad.y);

    Gfx::DrawFillRect(Gfx::FillRect((Gfx::Rect) {
        .x = 854 - pad.x - item.text.d.w,
        .y = y + pad.y,
        .d = {
            .w = item.text.d.w,
            .h = 64, //todo - make the freetype engine suck less
        },
    }, (item.id == selected_item) ? selected_colour : unselected_colour));
    item.text.Render(854 - pad.x - item.text.d.w, y + pad.y);

    return 64 + pad.y;
}

void Menu::Render() {
    Gfx::DrawFillRect(bg);
    int y = 0;

    y += DrawMenuItem(ip, y);
    y += DrawMenuItem(profile, y);
}

void Menu::Update(Config& config, const Input::WiiUInputState::Native& input) {
    if (config.menu_changed) {
        overlay.Change(config.networkconfig.host);
        ip.text.Change(config.networkconfig.host);
        profile.text.Change(std::to_string(config.currentProfile));
        config.menu_changed = false;
    }

    if (nn::swkbd::IsDecideCancelButton(nullptr)) {
        nn::swkbd::DisappearInputForm();
        swkbd_open = false;
    }

    if (nn::swkbd::IsDecideOkButton(nullptr)) {
        nn::swkbd::DisappearInputForm();
        swkbd_open = false;

        std::u16string utf16result(nn::swkbd::GetInputFormString());
        //this is apparently the best the C++ stdlib has to offer
        //thanks for removing the classes built specifically for this, c++17
        std::string result(std::begin(utf16result), std::end(utf16result));

        switch (selected_item) {
            case IP_ADDRESS: {
                config.networkconfig.host = result;
                config.Change();
                break;
            }
        }
    }

    if (swkbd_open) return;

    if (editing_item) {
        switch (selected_item) {
            case PROFILE: {
                if (input.vpad.trigger & VPAD_BUTTON_LEFT) {
                    config.currentProfile--;
                    config.Change();
                }
                if (input.vpad.trigger & VPAD_BUTTON_RIGHT) {
                    config.currentProfile++;
                    config.Change();
                }
            }
        }
        if (input.vpad.trigger & VPAD_BUTTON_A) {
            editing_item = false;
        }
    } else {
        if (input.vpad.trigger & VPAD_BUTTON_DOWN) {
            if (selected_item < MenuItemID::MAX - 1) selected_item++;
        }
        if (input.vpad.trigger & VPAD_BUTTON_UP) {
            if (selected_item > MenuItemID::NONE) selected_item--;
        }

        if (input.vpad.trigger & VPAD_BUTTON_A) {
            switch (selected_item) {
                case NONE: {
                    break;
                }
                case IP_ADDRESS: {
                    swkbd_open = nn::swkbd::AppearInputForm((nn::swkbd::AppearArg) {
                        .inputFormArg = {
                            .maxTextLength = 15,
                        },
                    });
                    break;
                }
                case PROFILE: {
                    editing_item = true;
                    break;
                }
            }
        }
    }
}
