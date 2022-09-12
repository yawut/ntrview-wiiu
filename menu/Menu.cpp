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
ip(u"3DS IP", IP_ADDRESS),
profile(u"Profile", PROFILE),
back_input_text(u"\uE001 Back"),
move_input_text(u"\uE07D Move"),
select_input_text(u"\uE07E Select"),
edit_input_text(u"\uE000 Edit")
{
    config.menu_changed = true;
    /*  Load background image */
    bg = Gfx::LoadFromJPEG(tj_handle, "fs:/vol/content/bgtexture.jpg");
}

const static Gfx::Rect pad = (Gfx::Rect) {
    .x = 60,
    .y = 30,
};
const static Gfx::Rect text_pad = (Gfx::Rect) {
    .x = 10,
    .y = 20,
};
const static int tri_size = 20;

static bool swkbd_open = false;
static int selected_item = IP_ADDRESS;

static bool profile_has_next = false;
static bool profile_has_prev = false;

int Menu::DrawMenuItem(MenuItem& item, int y) {
    int width = Gfx::GetCurrentScreenWidth();
    y += pad.y;

    Gfx::Rect btn_rect {
        .x = width / 4, .y = y,
        .d = { .w = width / 2, .h = item.label.d.h + item.text.d.h + text_pad.y * 3 },
    };
    Gfx::DrawFillRect(Gfx::FillRect {btn_rect, button_bg});

    item.label.Render((width - item.label.d.w) / 2, y + item.label.baseline_y + text_pad.y, text_colour);
    y += text_pad.y + item.label.d.h;
    item.text.Render((width - item.text.d.w) / 2, y + item.text.baseline_y + text_pad.y, sub_text_colour);
    y += text_pad.y + item.text.d.h;
    y += text_pad.y;

    if (selected_item == item.id) {
        DrawSelectionOutline(btn_rect);

        if (item.id == PROFILE) {
            int ty = btn_rect.y + (btn_rect.d.h / 2);
            if (profile_has_prev)
                Gfx::DrawFillTri(Gfx::FillTri {
                    Gfx::Tri {
                        .x = btn_rect.x - text_pad.x - tri_size, .y = ty,
                        .size = tri_size, .rotation = Gfx::GFX_ROTATION_90,
                    }, arrow_colour
                });
            if (profile_has_next)
                Gfx::DrawFillTri(Gfx::FillTri {
                    Gfx::Tri {
                        .x = btn_rect.x + btn_rect.d.w + text_pad.x + tri_size, .y = ty,
                        .size = tri_size, .rotation = Gfx::GFX_ROTATION_270,
                    }, arrow_colour
                });
        }
    }

    return y;
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

    int width = Gfx::GetCurrentScreenWidth();
    int height = Gfx::GetCurrentScreenHeight();

    back_input_text.Render(pad.x, height - pad.y, text_colour);
    y = height - pad.y;
    int w = std::max(std::max(move_input_text.d.w, select_input_text.d.w), edit_input_text.d.w);
    move_input_text.Render(width - pad.x - w, y, text_colour);
    y -= pad.y + move_input_text.d.h;
    if (selected_item == PROFILE) {
        select_input_text.Render(width - pad.x - w, y, text_colour);
    } else {
        edit_input_text.Render(width - pad.x - w, y, text_colour);
    }
}

const static Gfx::Dimensions outline_size = {.w = 30, .h = 10};

void Menu::DrawSelectionOutline(Gfx::Rect btnRect) {
    Gfx::FillRect rects[] {
        {(Gfx::Rect) {
            .x = btnRect.x,
            .y = btnRect.y,
            .d = {.w = outline_size.w, .h = outline_size.h},
        }, outline_colour},
        {(Gfx::Rect) {
            .x = btnRect.x,
            .y = btnRect.y,
            .d = {.w = outline_size.h, .h = outline_size.w},
        }, outline_colour},
        {(Gfx::Rect) {
            .x = btnRect.x + btnRect.d.w - outline_size.w,
            .y = btnRect.y,
            .d = {.w = outline_size.w, .h = outline_size.h},
        }, outline_colour},
        {(Gfx::Rect) {
            .x = btnRect.x + btnRect.d.w - outline_size.h,
            .y = btnRect.y,
            .d = {.w = outline_size.h, .h = outline_size.w},
        }, outline_colour},
        {(Gfx::Rect) {
            .x = btnRect.x,
            .y = btnRect.y + btnRect.d.h - outline_size.h,
            .d = {.w = outline_size.w, .h = outline_size.h},
        }, outline_colour},
        {(Gfx::Rect) {
            .x = btnRect.x,
            .y = btnRect.y + btnRect.d.h - outline_size.w,
            .d = {.w = outline_size.h, .h = outline_size.w},
        }, outline_colour},
        {(Gfx::Rect) {
            .x = btnRect.x + btnRect.d.w - outline_size.w,
            .y = btnRect.y + btnRect.d.h - outline_size.h,
            .d = {.w = outline_size.w, .h = outline_size.h},
        }, outline_colour},
        {(Gfx::Rect) {
            .x = btnRect.x + btnRect.d.w - outline_size.h,
            .y = btnRect.y + btnRect.d.h - outline_size.w,
            .d = {.w = outline_size.h, .h = outline_size.w},
        }, outline_colour},
    };
    for (const auto& rect : rects) {
        Gfx::DrawFillRect(rect);
    }
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
        ip.text.Change(to_u16string(config.networkconfig.host));
        profile.text.Change(to_u16string(config.profiles[config.profile].name));

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

        std::string result = to_string(nn::swkbd::GetInputFormString());

        switch (selected_item) {
            case IP_ADDRESS: {
                config.networkconfig.host = result;
                config.Change();
                break;
            }
        }
    }

    if (swkbd_open) return true;

    if (buttons & VPAD_BUTTON_DOWN) {
        if (selected_item < MenuItemID::MAX - 1) selected_item++;
    }
    if (buttons & VPAD_BUTTON_UP) {
        if (selected_item > MenuItemID::NONE) selected_item--;
    }

    if (buttons & VPAD_BUTTON_LEFT && selected_item == PROFILE) {
        config.PrevProfile();
    }
    if (buttons & VPAD_BUTTON_RIGHT && selected_item == PROFILE) {
        config.NextProfile();
    }

    if (buttons & VPAD_BUTTON_A && selected_item == IP_ADDRESS) {
        nn::swkbd::AppearArg kbd;
        kbd.keyboardArg.configArg.controllerType = get_controller(input.priority);
        kbd.keyboardArg.configArg.keyboardMode = nn::swkbd::KeyboardMode::Numpad;
        kbd.keyboardArg.configArg.numpadCharLeft = L'.';
        kbd.keyboardArg.configArg.disableNewLine = true;
        kbd.inputFormArg.type = nn::swkbd::InputFormType::InputForm0;
        kbd.inputFormArg.maxTextLength = 15;
        kbd.inputFormArg.hintText = u"Please enter your 3DS's IP address.";
        swkbd_open = nn::swkbd::AppearInputForm(kbd);
    }

    if (buttons & VPAD_BUTTON_B) {
        return false;
    }

    return true;
}
