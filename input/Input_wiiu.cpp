#include "Input.hpp"
#include "Input_wiiu.hpp"

#include <numbers>

#include <vpad/input.h>
#include <padscore/kpad.h>

#include "gfx/Gfx.hpp"

static Input::Priority kpad_prio(int ch) {
    switch (ch) {
        case 0: return Input::Priority::KPAD1;
        case 1: return Input::Priority::KPAD2;
        case 2: return Input::Priority::KPAD3;
        case 3: return Input::Priority::KPAD4;
        default: return Input::Priority::VPAD;
    }
}

//only send analog sticks for the last used controller
static Input::Priority current_priority = Input::Priority::VPAD;

std::optional<Input::WiiUInputState> Input::Get(Gfx::Rect touch_area) {
    Input::WiiUInputState input;
    bool priority_claimed = false;

    auto claimPriority = [&](Input::Priority p) {
        if (priority_claimed) return;

        priority_claimed = true;
        current_priority = p;
    };

    VPADReadError err = VPAD_READ_SUCCESS;
    VPADRead(VPAD_CHAN_0, &input.native.vpad, 1, &err);
    if (err == VPAD_READ_SUCCESS) {
        for (auto map : wiiu_button_map) {
            if ((input.native.vpad.hold & map.drc_btn) == map.drc_btn) {
                input.ds.buttons.press(map.ds_btn);
                claimPriority(Input::Priority::VPAD);
            }
        }
        for (auto map : wiiu_pro_map) {
            if ((input.native.vpad.hold & map.drc_btn) == map.drc_btn) {
                input.ds.pro.press(map.ds_btn);
                claimPriority(Input::Priority::VPAD);
            }
        }
        for (auto map : wiiu_buttons_sys_map) {
            if ((input.native.vpad.hold & map.drc_btn) == map.drc_btn) {
                input.ds.buttons_sys.press(map.ds_btn);
                claimPriority(Input::Priority::VPAD);
            }
        }

        if (input.native.vpad.tpNormal.touched) {
            VPADGetTPCalibratedPoint(VPAD_CHAN_0, &input.native.vpad.tpNormal, &input.native.vpad.tpNormal);

            float x = (float)input.native.vpad.tpNormal.x * 854.0f / 1280.0f;
            float y = (float)input.native.vpad.tpNormal.y * 480.0f / 720.0f;

            float fx = ((x - touch_area.x) / (float)touch_area.d.w);
            float fy = ((y - touch_area.y) / (float)touch_area.d.h);

            if (fx >= 0 && fx < 1 &&
                fy >= 0 && fy < 1) {

                input.ds.touch.x((uint16_t)(fx * 0xfff));
                input.ds.touch.y((uint16_t)(fy * 0xfff));
                input.ds.touch.flags(0x01);
            }
        }

        if (current_priority == Input::Priority::VPAD) {
            using namespace std::numbers;

            int16_t lx = (int16_t)( input.native.vpad.leftStick.x * 0x5d0) + 0x800;
            int16_t clx = lx & 0xf80; //get mad!
            input.ds.circle.x(clx);
            int16_t ly = (int16_t)( input.native.vpad.leftStick.y * 0x5d0) + 0x800;
            int16_t cly = ly & 0xf80; //don't make lemonade!
            input.ds.circle.y(cly);

            auto rx = ( input.native.vpad.rightStick.x + input.native.vpad.rightStick.y ) / sqrt2;
            uint8_t crx = ( rx * 0x7f ) + 0x80;
            input.ds.pro.x(crx);
            auto ry = ( input.native.vpad.rightStick.y - input.native.vpad.rightStick.x ) / sqrt2;
            uint8_t cry = ( ry * 0x7f ) + 0x80;
            input.ds.pro.y(cry);
        }
    }

    for (int i = 0; i < 4; i++) {
        KPADStatus& kpad = input.native.kpad[i];

        int32_t err;
        int res = KPADReadEx((KPADChan)i, &kpad, 1, &err);
        if (res == 0) {
            kpad.error = 1;
            continue;
        } else if (err != KPAD_ERROR_OK) {
            kpad.error = err;
            continue;
        }

        uint32_t buttons;
        KPADVec2D leftStick = {0, 0};
        KPADVec2D rightStick = {0, 0};
        Input::ExtType ext = MapForKPADExt(kpad.extensionType);

        switch (kpad.extensionType) {
            default:
            case WPAD_EXT_CORE:
            case WPAD_EXT_MPLUS: {
                buttons = kpad.hold;
                break;
            }
            case WPAD_EXT_NUNCHUK:
            case WPAD_EXT_MPLUS_NUNCHUK: {
                //what are you even doing
                buttons = kpad.hold;
                leftStick = kpad.nunchuck.stick;
                //rightStick = kpad.nunchuck.acc; one day I shall curse you all
                break;
            }
            case WPAD_EXT_CLASSIC:
            case WPAD_EXT_MPLUS_CLASSIC: {
                buttons = kpad.classic.hold;
                leftStick = kpad.classic.leftStick;
                rightStick = kpad.classic.rightStick;
                break;
            }
            case WPAD_EXT_PRO_CONTROLLER: {
                buttons = kpad.pro.hold;
                leftStick = kpad.pro.leftStick;
                rightStick = kpad.pro.rightStick;
                break;
            }
        }

        for (auto map : wiiu_button_map) {
            if (buttons & map.kpad_btn[(int)ext]) {
                input.ds.buttons.press(map.ds_btn);
                claimPriority(kpad_prio(i));
            }
        }

        for (auto map : wiiu_pro_map) {
            if (buttons & map.kpad_btn[(int)ext]) {
                input.ds.pro.press(map.ds_btn);
                claimPriority(kpad_prio(i));
            }
        }

        for (auto map : wiiu_buttons_sys_map) {
            if (buttons & map.kpad_btn[(int)ext]) {
                input.ds.buttons_sys.press(map.ds_btn);
                claimPriority(kpad_prio(i));
            }
        }

        //kinda reusing this code from vpad and hoping it works
        if (current_priority == kpad_prio(i)) {
            using namespace std::numbers;

            int16_t lx = (int16_t)( leftStick.x * 0x5d0) + 0x800;
            int16_t clx = lx & 0xf80; //get mad!
            input.ds.circle.x(clx);
            int16_t ly = (int16_t)( leftStick.y * 0x5d0) + 0x800;
            int16_t cly = ly & 0xf80; //don't make lemonade!
            input.ds.circle.y(cly);

            auto rx = ( rightStick.x + rightStick.y ) / sqrt2;
            uint8_t crx = ( rx * 0x7f ) + 0x80;
            input.ds.pro.x(crx);
            auto ry = ( rightStick.y - rightStick.x ) / sqrt2;
            uint8_t cry = ( ry * 0x7f ) + 0x80;
            input.ds.pro.y(cry);
        }
    }

    input.priority = current_priority;
    //nobody sees bad code if it works
    switch (input.priority) {
        case Input::Priority::VPAD:
        default: {
            input.ext = Input::ExtType::Core; //whatever
            break;
        }
        case Input::Priority::KPAD1: {
            input.ext = MapForKPADExt(input.native.kpad[0].extensionType);
            break;
        }
        case Input::Priority::KPAD2: {
            input.ext = MapForKPADExt(input.native.kpad[1].extensionType);
            break;
        }
        case Input::Priority::KPAD3: {
            input.ext = MapForKPADExt(input.native.kpad[2].extensionType);
            break;
        }
        case Input::Priority::KPAD4: {
            input.ext = MapForKPADExt(input.native.kpad[3].extensionType);
            break;
        }
    }
    return input;
}
