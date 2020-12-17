#pragma once

#include "gfx/Gfx.hpp"

#include <vector>
#include <string>
#include <fstream>
#include <stdint.h>

class Config {
public:
    void LoadINI(std::basic_istream<char>& is);
    void SaveINI(std::basic_ostream<char>& os);

    struct NetworkConfig {
        std::string host = "";
        uint8_t priority = 1;
        uint8_t priorityFactor = 5;
        uint8_t jpegQuality = 80;
        uint8_t QoS = 18;

        int input_ratelimit_us = 50 * 1000;
        int input_pollrate_us = 5 * 1000;
    };
    NetworkConfig networkconfig;

    Gfx::rgb background = {
        .r = 0x20,
        .g = 0x00,
        .b = 0x27,
    };

    int profile = 0;
    struct Profile {
        Gfx::Rect layout_tv[Gfx::RESOLUTION_MAX][2 /*inputs*/];
        Gfx::Rect layout_drc[2];
    };
    std::vector<Profile> profiles = { (Profile) {
        .layout_tv = {
            [Gfx::RESOLUTION_480P] = {
                [0] = (Gfx::Rect) {
                    .x = 27,
                    .y = 0,
                    .d = {
                        .w = 800,
                        .h = 480,
                    },
                    .rotation = Gfx::GFX_ROTATION_270,
                },
                [1] = (Gfx::Rect) {
                    .d = {
                        .w = 0,
                    },
                    .rotation = Gfx::GFX_ROTATION_270,
                },
            },
            [Gfx::RESOLUTION_720P] = {
                [0] = (Gfx::Rect) {
                    .x = 40,
                    .y = 0,
                    .d = {
                        .w = 1200,
                        .h = 720,
                    },
                    .rotation = Gfx::GFX_ROTATION_270,
                },
                [1] = (Gfx::Rect) {
                    .d = {
                        .w = 0,
                    },
                    .rotation = Gfx::GFX_ROTATION_270,
                },
            },
            [Gfx::RESOLUTION_1080P] = {
                [0] = (Gfx::Rect) {
                    .x = 60,
                    .y = 0,
                    .d = {
                        .w = 1800,
                        .h = 1080,
                    },
                    .rotation = Gfx::GFX_ROTATION_270,
                },
                [1] = (Gfx::Rect) {
                    .d = {
                        .w = 0,
                    },
                    .rotation = Gfx::GFX_ROTATION_270,
                },
            },
        },

        .layout_drc = {
            [0] = (Gfx::Rect) {
                .d = {
                    .w = 0,
                },
                .rotation = Gfx::GFX_ROTATION_270,
            },
            [1] = (Gfx::Rect) {
                .x = 107,
                .y = 0,
                .d = {
                    .w = 640,
                    .h = 480,
                },
                .rotation = Gfx::GFX_ROTATION_270,
            },
        }
    }};
};
