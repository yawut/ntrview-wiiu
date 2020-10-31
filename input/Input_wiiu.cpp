#include "Input.hpp"
#include "Input_wiiu.hpp"

#include <vpad/input.h>

#include "gfx/Gfx.hpp"

std::optional<Input::InputState> Input::Get(Gfx::Rect touch_area) {
    Input::InputState input;

    VPADStatus status;
    VPADReadError err = VPAD_READ_SUCCESS;
    VPADRead(VPAD_CHAN_0, &status, 1, &err);
    if (err == VPAD_READ_SUCCESS) {
        for (auto map : wiiu_button_map) {
            if ((status.hold & map.drc_btn) == map.drc_btn) {
                input.buttons.press(map.ds_btn);
            }
        }
        for (auto map : wiiu_pro_map) {
            if ((status.hold & map.drc_btn) == map.drc_btn) {
                input.pro.press(map.ds_btn);
            }
        }
        /*for (auto map : wiiu_buttons_sys_map) {
            if ((status.hold & map.drc_btn) == map.drc_btn) {
                input.buttons_sys.press(map.ds_btn);
            }
        }*/

        int16_t lx = (int16_t)( status.leftStick.x * 0x5d0) + 0x800;
        int16_t clx = lx & 0xf80; //get mad!
        input.circle.x(clx);
        int16_t ly = (int16_t)( status.leftStick.y * 0x5d0) + 0x800;
        int16_t cly = ly & 0xf80; //don't make lemonade!
        input.circle.y(cly);

        if (status.tpNormal.touched) {
            VPADTouchData touch;
            VPADGetTPCalibratedPoint(VPAD_CHAN_0, &touch, &status.tpNormal);

            float x = (float)touch.x * 854.0f / 1280.0f;
            float y = (float)touch.y * 480.0f / 720.0f;

            float fx = ((x - touch_area.x) / (float)touch_area.d.w);
            float fy = ((y - touch_area.y) / (float)touch_area.d.h);

            if (fx >= 0 && fx < 1 &&
                fy >= 0 && fy < 1) {

                input.touch.x((uint16_t)(fx * 0xfff));
                input.touch.y((uint16_t)(fy * 0xfff));
                input.touch.flags(0x01);
            }
        }

        return input;
    }

    return std::nullopt;
}
