#pragma once

#include <optional>
#include <stdint.h>

#include "gfx/Gfx.hpp"

namespace Input {

typedef enum Buttons {
    DS_BUTTON_A = 0,
    DS_BUTTON_B = 1,
    DS_BUTTON_SELECT = 2,
    DS_BUTTON_START = 3,
    DS_BUTTON_RIGHT = 4,
    DS_BUTTON_LEFT = 5,
    DS_BUTTON_UP = 6,
    DS_BUTTON_DOWN = 7,
    DS_BUTTON_R = 8,
    DS_BUTTON_L = 9,
    DS_BUTTON_X = 10,
    DS_BUTTON_Y = 11,
} Buttons;
typedef enum ZButtons {
    DS_BUTTON_ZR = 1, //????
    DS_BUTTON_ZL = 2,
} ZButtons;
typedef enum SButtons {
    DS_BUTTON_HOME = 0,
    DS_BUTTON_POWER = 1,
} SButtons;

class InputState {
public:
    struct buttons {
        uint32_t data = 0xfff;
        void press(Buttons btn) {
            data &= ~(1 << btn);
        }
    } buttons;
    typedef struct {
        uint32_t data;
        void flags(uint8_t flag) {
            data &= ~(0xFF << 24);
            data |=  (flag << 24);
        }
        void y(uint16_t y) {
            data &= ~(0xFFF << 12);
            data |=  ((y & 0xFFF) << 12);
        }
        void x(uint16_t x) {
            data &= ~0xFFF;
            data |= x & 0xFFF;
        }
    } coords;
    coords touch { .data = 0x2000000 };
    coords circle { .data = 0x07ff7ff };
    struct {
        uint32_t data = 0x80800081;
        void y(uint8_t y) {
            data &= ~(0xFF << 24);
            data |=  (y    << 24);
        }
        void x(uint8_t x) {
            data &= ~(0xFF << 16);
            data |=  (x    << 16);
        }
        void press(ZButtons btn) {
            data |= 1 << (btn+8);
        }
        void magic(uint8_t val) {
            data &= ~0xFF;
            data |=  val;
        }
    } pro;
    struct {
        uint32_t data = 0;
        void press(SButtons btn) {
            data |= 1 << btn;
        }
    } buttons_sys;

    bool operator==(const InputState& rhs) {
        return this->buttons.data == rhs.buttons.data &&
            this->touch.data == rhs.touch.data &&
            this->circle.data == rhs.circle.data &&
            this->pro.data == rhs.pro.data &&
            this->buttons_sys.data == rhs.buttons_sys.data;
    }
};

/*
    00..000YXLRDULRSSBA
    000000??-12y-12x
    00000000-12y-12x
    8y-8x-8b-x81
    0..00PlPH
*/

std::optional<InputState> Get(Gfx::Rect touch_area);

} //namespace Input
