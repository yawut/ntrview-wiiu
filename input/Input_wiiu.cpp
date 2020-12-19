#include "Input.hpp"
#include "Input_wiiu.hpp"

#include <numbers>

#include <vpad/input.h>

#include "gfx/Gfx.hpp"

std::optional<Input::WiiUInputState> Input::Get(Gfx::Rect touch_area) {
    Input::WiiUInputState input;

    VPADReadError err = VPAD_READ_SUCCESS;
    VPADRead(VPAD_CHAN_0, &input.native.vpad, 1, &err);
    if (err == VPAD_READ_SUCCESS) {
        for (auto map : wiiu_button_map) {
            if ((input.native.vpad.hold & map.drc_btn) == map.drc_btn) {
                input.ds.buttons.press(map.ds_btn);
            }
        }
        for (auto map : wiiu_pro_map) {
            if ((input.native.vpad.hold & map.drc_btn) == map.drc_btn) {
                input.ds.pro.press(map.ds_btn);
            }
        }
        for (auto map : wiiu_buttons_sys_map) {
            if ((input.native.vpad.hold & map.drc_btn) == map.drc_btn) {
                input.ds.buttons_sys.press(map.ds_btn);
            }
        }

        {
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

        return input;
    }

    return std::nullopt;
}
