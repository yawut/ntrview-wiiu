#pragma once

#include "Input.hpp"

#include <map>
#include <vpad/input.h>
#include <padscore/kpad.h>

static Input::ExtType MapForKPADExt(uint8_t ext) {
    switch (ext) {
        case WPAD_EXT_CORE: return Input::ExtType::Core;
        case WPAD_EXT_MPLUS: return Input::ExtType::Core;

        case WPAD_EXT_NUNCHUK: return Input::ExtType::Nunchuk;
        case WPAD_EXT_MPLUS_NUNCHUK: return Input::ExtType::Nunchuk;

        case WPAD_EXT_CLASSIC: return Input::ExtType::Classic;
        case WPAD_EXT_MPLUS_CLASSIC: return Input::ExtType::Classic;

        case WPAD_EXT_PRO_CONTROLLER: return Input::ExtType::Pro;
        default: return Input::ExtType::Core;
    }
}

#define NOT_BOUND 0

const static struct {
    Input::Buttons ds_btn;
    uint32_t drc_btn = 0;
    std::array<uint32_t, (int)Input::ExtType::_NumExts> kpad_btn; //index with MapKPADExt
} wiiu_button_map[] = {
    {
        .ds_btn = Input::DS_BUTTON_A,
        .drc_btn = VPAD_BUTTON_A,
        .kpad_btn = { WPAD_BUTTON_2, NOT_BOUND, WPAD_CLASSIC_BUTTON_A, WPAD_PRO_BUTTON_A, }
    },
    {
        .ds_btn = Input::DS_BUTTON_B,
        .drc_btn = VPAD_BUTTON_B,
        .kpad_btn = { WPAD_BUTTON_1, NOT_BOUND, WPAD_CLASSIC_BUTTON_B, WPAD_PRO_BUTTON_B, }
    },
    {
        .ds_btn = Input::DS_BUTTON_START,
        .drc_btn = VPAD_BUTTON_PLUS,
        .kpad_btn = { WPAD_BUTTON_PLUS, NOT_BOUND, WPAD_CLASSIC_BUTTON_PLUS, WPAD_PRO_BUTTON_PLUS, }
    },
    {
        .ds_btn = Input::DS_BUTTON_SELECT,
        .drc_btn = VPAD_BUTTON_MINUS,
        .kpad_btn = { WPAD_BUTTON_MINUS, NOT_BOUND, WPAD_CLASSIC_BUTTON_MINUS, WPAD_PRO_BUTTON_MINUS, }
    },
    {
        .ds_btn = Input::DS_BUTTON_RIGHT,
        .drc_btn = VPAD_BUTTON_RIGHT,
        .kpad_btn = { WPAD_BUTTON_DOWN, NOT_BOUND, WPAD_CLASSIC_BUTTON_RIGHT, WPAD_PRO_BUTTON_RIGHT, }
    },
    {
        .ds_btn = Input::DS_BUTTON_LEFT,
        .drc_btn = VPAD_BUTTON_LEFT,
        .kpad_btn = { WPAD_BUTTON_UP, NOT_BOUND, WPAD_CLASSIC_BUTTON_LEFT, WPAD_PRO_BUTTON_LEFT, }
    },
    {
        .ds_btn = Input::DS_BUTTON_UP,
        .drc_btn = VPAD_BUTTON_UP,
        .kpad_btn = { WPAD_BUTTON_RIGHT, NOT_BOUND, WPAD_CLASSIC_BUTTON_UP, WPAD_PRO_BUTTON_UP, }
    },
    {
        .ds_btn = Input::DS_BUTTON_DOWN,
        .drc_btn = VPAD_BUTTON_DOWN,
        .kpad_btn = { WPAD_BUTTON_LEFT, NOT_BOUND, WPAD_CLASSIC_BUTTON_DOWN, WPAD_PRO_BUTTON_DOWN, }
    },
    {
        .ds_btn = Input::DS_BUTTON_R,
        .drc_btn = VPAD_BUTTON_R,
        .kpad_btn = { NOT_BOUND, NOT_BOUND, WPAD_CLASSIC_BUTTON_R, WPAD_PRO_TRIGGER_R, }
    },
    {
        .ds_btn = Input::DS_BUTTON_L,
        .drc_btn = VPAD_BUTTON_L,
        .kpad_btn = { NOT_BOUND, NOT_BOUND, WPAD_CLASSIC_BUTTON_L, WPAD_PRO_TRIGGER_L, }
    },
    {
        .ds_btn = Input::DS_BUTTON_X,
        .drc_btn = VPAD_BUTTON_X,
        .kpad_btn = { WPAD_BUTTON_A, NOT_BOUND, WPAD_CLASSIC_BUTTON_X, WPAD_PRO_BUTTON_X, }
    },
    {
        .ds_btn = Input::DS_BUTTON_Y,
        .drc_btn = VPAD_BUTTON_Y,
        .kpad_btn = { WPAD_BUTTON_B, NOT_BOUND, WPAD_CLASSIC_BUTTON_Y, WPAD_PRO_BUTTON_Y, }
    },
};

const static struct {
    Input::ZButtons ds_btn;
    uint32_t drc_btn = 0;
    std::array<uint32_t, (int)Input::ExtType::_NumExts> kpad_btn; //index with MapKPADExt
} wiiu_pro_map[] = {
    {
        .ds_btn = Input::DS_BUTTON_ZL,
        .drc_btn = VPAD_BUTTON_ZL,
        .kpad_btn = { NOT_BOUND, NOT_BOUND, WPAD_CLASSIC_BUTTON_ZL, WPAD_PRO_TRIGGER_ZL, }
    },
    {
        .ds_btn = Input::DS_BUTTON_ZR,
        .drc_btn = VPAD_BUTTON_ZR,
        .kpad_btn = { NOT_BOUND, NOT_BOUND, WPAD_CLASSIC_BUTTON_ZR, WPAD_PRO_TRIGGER_ZR, }
    },
};

const static struct {
    Input::SButtons ds_btn;
    uint32_t drc_btn = 0;
    std::array<uint32_t, (int)Input::ExtType::_NumExts> kpad_btn; //index with MapKPADExt
} wiiu_buttons_sys_map[] = {
    {
        //stopgap until real input mapping and/or menus are a thing
        .ds_btn = Input::DS_BUTTON_HOME,
        .drc_btn = VPAD_BUTTON_STICK_R,
        .kpad_btn = { NOT_BOUND, NOT_BOUND, NOT_BOUND, NOT_BOUND, }
    },
};
