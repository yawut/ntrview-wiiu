#pragma once

#include <map>
#include <vpad/input.h>

const static struct {
    Input::Buttons ds_btn;
    uint32_t drc_btn;
} wiiu_button_map[] = {
    {
        .ds_btn = Input::DS_BUTTON_A,
        .drc_btn = VPAD_BUTTON_A,
    },
    {
        .ds_btn = Input::DS_BUTTON_B,
        .drc_btn = VPAD_BUTTON_B,
    },
    {
        .ds_btn = Input::DS_BUTTON_START,
        .drc_btn = VPAD_BUTTON_PLUS,
    },
    {
        .ds_btn = Input::DS_BUTTON_SELECT,
        .drc_btn = VPAD_BUTTON_MINUS,
    },
    {
        .ds_btn = Input::DS_BUTTON_RIGHT,
        .drc_btn = VPAD_BUTTON_RIGHT,
    },
    {
        .ds_btn = Input::DS_BUTTON_LEFT,
        .drc_btn = VPAD_BUTTON_LEFT,
    },
    {
        .ds_btn = Input::DS_BUTTON_UP,
        .drc_btn = VPAD_BUTTON_UP,
    },
    {
        .ds_btn = Input::DS_BUTTON_DOWN,
        .drc_btn = VPAD_BUTTON_DOWN,
    },
    {
        .ds_btn = Input::DS_BUTTON_R,
        .drc_btn = VPAD_BUTTON_R,
    },
    {
        .ds_btn = Input::DS_BUTTON_L,
        .drc_btn = VPAD_BUTTON_L,
    },
    {
        .ds_btn = Input::DS_BUTTON_X,
        .drc_btn = VPAD_BUTTON_X,
    },
    {
        .ds_btn = Input::DS_BUTTON_Y,
        .drc_btn = VPAD_BUTTON_Y,
    },
};

const static struct {
    Input::ZButtons ds_btn;
    uint32_t drc_btn;
} wiiu_pro_map[] = {
    {
        .ds_btn = Input::DS_BUTTON_ZL,
        .drc_btn = VPAD_BUTTON_ZL,
    },
    {
        .ds_btn = Input::DS_BUTTON_ZR,
        .drc_btn = VPAD_BUTTON_ZR,
    },
};
