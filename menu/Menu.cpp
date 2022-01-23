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
            .w = 1920,
            .h = 1080,
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
const static Gfx::Rect text_pad = (Gfx::Rect) {
    .x = 5,
};

static bool swkbd_open = false;
static int selected_item = IP_ADDRESS;
static bool editing_item = false;

static bool profile_has_next = false;
static bool profile_has_prev = false;

int Menu::DrawMenuItem(MenuItem& item, int y) {
    int width = Gfx::GetCurrentScreenWidth();
    item.label.Render(pad.x, y + pad.y + item.label.pt_size);

    int box_h = (item.text.pt_size * 4) / 3;

    Gfx::DrawFillRect(Gfx::FillRect((Gfx::Rect) {
        .x = width - pad.x - item.text.d.w - text_pad.x,
        .y = y + pad.y,
        .d = {
            .w = item.text.d.w + text_pad.x*2,
            .h = box_h,
        },
    }, (item.id == selected_item) ? selected_colour : unselected_colour));
    item.text.Render(width - pad.x - item.text.d.w, y + pad.y + item.text.pt_size);

    if (item.id == selected_item && item.id == PROFILE) {
        int ty = y + pad.y + box_h / 2;
        Gfx::DrawFillTri(Gfx::FillTri((Gfx::Tri) {
            .x = width - pad.x - item.text.d.w - (pad.x * 3 / 4),
            .y = ty,
            .size = pad.x / 2,
            .rotation = Gfx::GFX_ROTATION_90,
        }, (editing_item && profile_has_prev)? selected_colour : unselected_colour));
        Gfx::DrawFillTri(Gfx::FillTri((Gfx::Tri) {
            .x = width - (pad.x / 4),
            .y = ty,
            .size = pad.x / 2,
            .rotation = Gfx::GFX_ROTATION_270,
        }, (editing_item && profile_has_next)? selected_colour : unselected_colour));
    }

    return box_h + pad.y;
}

void Menu::Render() {
    Gfx::DrawFillRect(bg);
    int y = 0;

    y += DrawMenuItem(ip, y);
    y += DrawMenuItem(profile, y);
}

bool Menu::Update(Config& config, const Input::WiiUInputState::Native& input) {
    if (config.menu_changed) {
        overlay.ChangeHost(config.networkconfig.host);
        ip.text.Change(config.networkconfig.host);
        profile.text.Change(config.profiles[config.profile].name);

        profile_has_next = config.GetNextProfile().has_value();
        profile_has_prev = config.GetPrevProfile().has_value();

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

    if (swkbd_open) return true;

    if (editing_item) {
        switch (selected_item) {
            case PROFILE: {
                if (input.vpad.trigger & VPAD_BUTTON_LEFT) {
                    config.PrevProfile();
                }
                if (input.vpad.trigger & VPAD_BUTTON_RIGHT) {
                    config.NextProfile();
                }
            }
        }
        if (input.vpad.trigger & (VPAD_BUTTON_A | VPAD_BUTTON_B)) {
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
                    nn::swkbd::AppearArg kbd;
                    kbd.keyboardArg.configArg.keyboardMode = nn::swkbd::KeyboardMode::Numpad;
                    kbd.keyboardArg.configArg.numpadCharLeft = L'.';
                    kbd.keyboardArg.configArg.disableNewLine = true;
                    kbd.inputFormArg.type = nn::swkbd::InputFormType::inputform0;
                    kbd.inputFormArg.maxTextLength = 15;
                    kbd.inputFormArg.hintText = u"Please enter your 3DS's IP address.";
                    swkbd_open = nn::swkbd::AppearInputForm(kbd);
                    break;
                }
                case PROFILE: {
                    editing_item = true;
                    break;
                }
            }
        }

        if (input.vpad.trigger & VPAD_BUTTON_B) {
            return false;
        }
    }

    return true;
}
