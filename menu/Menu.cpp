#include "Menu.hpp"
#include "gfx/JPEG.h"

#include <nn/swkbd.h>
#include <string>
#include <locale>

enum MenuItemID {
    NONE = 0,
    IP_ADDRESS,
    PROFILE,
    MAX,
};

Menu::Menu(Config& config, tjhandle tj_handle) :
overlay(config.networkconfig.host),
ip("3DS IP:", IP_ADDRESS),
profile("Profile:", PROFILE) {
    config.menu_changed = true;
    /*  Load background image */
    bg = Gfx::LoadFromJPEG(tj_handle, "fs:/vol/content/bgtexture.jpg");
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
    item.label.Render(pad.x, y + pad.y + item.label.pt_size, text_colour);

    int box_h = (item.text.pt_size * 4) / 3;

    Gfx::DrawFillRect(Gfx::FillRect((Gfx::Rect) {
        .x = width - pad.x - item.text.d.w - text_pad.x,
        .y = y + pad.y,
        .d = {
            .w = item.text.d.w + text_pad.x*2,
            .h = box_h,
        },
    }, (item.id == selected_item) ? selected_colour : unselected_colour));
    item.text.Render(width - pad.x - item.text.d.w, y + pad.y + item.text.pt_size, text_colour);

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
    bg.Render((Gfx::Rect) {
        .d = {
            .w = Gfx::GetCurrentScreenWidth(),
            .h = Gfx::GetCurrentScreenHeight(),
        },
    });
    int y = 0;

    y += DrawMenuItem(ip, y);
    y += DrawMenuItem(profile, y);
}

const static std::pair<uint32_t, VPADButtons> wiimote_menu_map[] = {
    { WPAD_BUTTON_UP, VPAD_BUTTON_LEFT },
    { WPAD_BUTTON_DOWN, VPAD_BUTTON_RIGHT },
    { WPAD_BUTTON_LEFT, VPAD_BUTTON_DOWN },
    { WPAD_BUTTON_RIGHT, VPAD_BUTTON_UP },
    { WPAD_BUTTON_1, VPAD_BUTTON_B },
    { WPAD_BUTTON_2, VPAD_BUTTON_A },
    //{ WPAD_BUTTON_PLUS | WPAD_BUTTON_MINUS, VPAD_BUTTON_STICK_L },
};
const static std::pair<uint32_t, VPADButtons> classic_menu_map[] = {
    { WPAD_CLASSIC_BUTTON_LEFT, VPAD_BUTTON_LEFT },
    { WPAD_CLASSIC_BUTTON_RIGHT, VPAD_BUTTON_RIGHT },
    { WPAD_CLASSIC_BUTTON_DOWN, VPAD_BUTTON_DOWN },
    { WPAD_CLASSIC_BUTTON_UP, VPAD_BUTTON_UP },
    { WPAD_CLASSIC_BUTTON_B, VPAD_BUTTON_B },
    { WPAD_CLASSIC_BUTTON_A, VPAD_BUTTON_A },
    //{ WPAD_CLASSIC_BUTTON_PLUS | WPAD_CLASSIC_BUTTON_MINUS, VPAD_BUTTON_STICK_L },
};
const static std::pair<uint32_t, VPADButtons> pro_menu_map[] = {
    { WPAD_PRO_BUTTON_LEFT, VPAD_BUTTON_LEFT },
    { WPAD_PRO_BUTTON_RIGHT, VPAD_BUTTON_RIGHT },
    { WPAD_PRO_BUTTON_DOWN, VPAD_BUTTON_DOWN },
    { WPAD_PRO_BUTTON_UP, VPAD_BUTTON_UP },
    { WPAD_PRO_BUTTON_B, VPAD_BUTTON_B },
    { WPAD_PRO_BUTTON_A, VPAD_BUTTON_A },
    { WPAD_PRO_BUTTON_STICK_L, VPAD_BUTTON_STICK_L },
};

static uint32_t translateButtons(const Input::WiiUInputState::Native& input) {
    uint32_t buttons = input.vpad.trigger;

    for (const auto& kpad : input.kpad) {
        if (kpad.error != KPAD_ERROR_OK) continue;

        switch (kpad.extensionType) {
            case WPAD_EXT_CORE:
            case WPAD_EXT_MPLUS:
            case WPAD_EXT_NUNCHUK:
            case WPAD_EXT_MPLUS_NUNCHUK: {
                for (const auto& btn : wiimote_menu_map) {
                    auto [kpad_btn, vpad_btn] = btn;
                    if ((kpad.trigger & kpad_btn) == kpad_btn) buttons |= vpad_btn;
                }
                //special case: use hold instead of trigger for multi-button combos
                if ((kpad.hold & (WPAD_BUTTON_PLUS | WPAD_BUTTON_MINUS)) ==
                    (WPAD_BUTTON_PLUS | WPAD_BUTTON_MINUS)) {
                    buttons |= VPAD_BUTTON_STICK_L;
                }
                break;
            }
            case WPAD_EXT_CLASSIC:
            case WPAD_EXT_MPLUS_CLASSIC: {
                for (const auto& btn : classic_menu_map) {
                    auto [kpad_btn, vpad_btn] = btn;
                    if ((kpad.classic.trigger & kpad_btn) == kpad_btn) buttons |= vpad_btn;
                }
                //special case: use hold instead of trigger for multi-button combos
                if ((kpad.classic.hold & (WPAD_CLASSIC_BUTTON_PLUS | WPAD_CLASSIC_BUTTON_MINUS)) ==
                    (WPAD_CLASSIC_BUTTON_PLUS | WPAD_CLASSIC_BUTTON_MINUS)) {
                    buttons |= VPAD_BUTTON_STICK_L;
                }
                break;
            }
            case WPAD_EXT_PRO_CONTROLLER: {
                for (const auto& btn : pro_menu_map) {
                    auto [kpad_btn, vpad_btn] = btn;
                    if ((kpad.pro.trigger & kpad_btn) == kpad_btn) buttons |= vpad_btn;
                }
                break;
            }
        }
    }

    return buttons;
}

static nn::swkbd::ControllerType get_controller(Input::Priority controller) {
    switch (controller) {
        case Input::Priority::VPAD: return nn::swkbd::ControllerType::DrcGamepad;
        case Input::Priority::KPAD1: return nn::swkbd::ControllerType::WiiRemote0;
        case Input::Priority::KPAD2: return nn::swkbd::ControllerType::WiiRemote1;
        case Input::Priority::KPAD3: return nn::swkbd::ControllerType::WiiRemote2;
        case Input::Priority::KPAD4: return nn::swkbd::ControllerType::WiiRemote3;
        default: return nn::swkbd::ControllerType::DrcGamepad;
    }
}

bool Menu::Update(Config& config, bool open, const Input::WiiUInputState& input) {
    auto buttons = translateButtons(input.native);

    if (!open) {
        if (buttons & VPAD_BUTTON_STICK_L) return true;
        return false;
    }

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
                if (buttons & VPAD_BUTTON_LEFT) {
                    config.PrevProfile();
                }
                if (buttons & VPAD_BUTTON_RIGHT) {
                    config.NextProfile();
                }
            }
        }
        if (buttons & (VPAD_BUTTON_A | VPAD_BUTTON_B)) {
            editing_item = false;
        }
    } else {
        if (buttons & VPAD_BUTTON_DOWN) {
            if (selected_item < MenuItemID::MAX - 1) selected_item++;
        }
        if (buttons & VPAD_BUTTON_UP) {
            if (selected_item > MenuItemID::NONE) selected_item--;
        }

        if (buttons & VPAD_BUTTON_A) {
            switch (selected_item) {
                default:
                case NONE: {
                    break;
                }
                case IP_ADDRESS: {
                    nn::swkbd::AppearArg kbd;
                    kbd.keyboardArg.configArg.controllerType = get_controller(input.priority);
                    kbd.keyboardArg.configArg.keyboardMode = nn::swkbd::KeyboardMode::Numpad;
                    kbd.keyboardArg.configArg.numpadCharLeft = L'.';
                    kbd.keyboardArg.configArg.disableNewLine = true;
                    kbd.inputFormArg.type = nn::swkbd::InputFormType::InputForm0;
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

        if (buttons & VPAD_BUTTON_B) {
            return false;
        }
    }

    return true;
}
