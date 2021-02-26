#pragma once

#include "gfx/Gfx.hpp"

#include <vector>
#include <string>
#include <fstream>
#include <stdint.h>

class Config {
public:
    bool changed = false;
    bool menu_changed = false;
    void Change() { changed = true; menu_changed = true; }
    void LoadINI(std::basic_istream<char>& is);
    void SaveINI(std::basic_ostream<char>& os);

    //stuff saved to config
    struct NetworkConfig {
        std::string host = "unset";
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

    size_t profile = 0;
    std::optional<size_t> GetNextProfile() {
        if (this->profile < this->profiles.size() - 1) {
            return this->profile + 1;
        } else return std::nullopt;
    }
    bool NextProfile() {
        if (auto prof = GetNextProfile()) {
            this->profile = *prof;
            this->Change();
            return true;
        } else return false;
    }
    std::optional<size_t> GetPrevProfile() {
        if (this->profile > 0) {
            return this->profile - 1;
        } else return std::nullopt;
    }
    bool PrevProfile() {
        if (auto prof = GetPrevProfile()) {
            this->profile = *prof;
            this->Change();
            return true;
        } else return false;
    }

    struct Profile {
        std::string name;
        Gfx::Rect layout_tv[Gfx::RESOLUTION_MAX][2 /*inputs*/];
        Gfx::Rect layout_drc[2];
    };
    std::vector<Profile> profiles = { (Profile) {
        .name = "Large Screens",
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
    }, (Profile) {
        .name = "Both Screens",
        .layout_tv = {
            [Gfx::RESOLUTION_480P] = {
                [0] = (Gfx::Rect) {
                    .x = 227,
                    .y = 0,
                    .d = {
                        .w = 400,
                        .h = 240,
                    },
                    .rotation = Gfx::GFX_ROTATION_270,
                },
                [1] = (Gfx::Rect) {
                    .x = 267,
                    .y = 240,
                    .d = {
                        .w = 320,
                        .h = 240,
                    },
                    .rotation = Gfx::GFX_ROTATION_270,
                },
            },
            [Gfx::RESOLUTION_720P] = {
                [0] = (Gfx::Rect) {
                    .x = 240,
                    .y = 0,
                    .d = {
                        .w = 800,
                        .h = 480,
                    },
                    .rotation = Gfx::GFX_ROTATION_270,
                },
                [1] = (Gfx::Rect) {
                    .x = 480,
                    .y = 480,
                    .d = {
                        .w = 320,
                        .h = 240,
                    },
                    .rotation = Gfx::GFX_ROTATION_270,
                },
            },
            [Gfx::RESOLUTION_1080P] = {
                [0] = (Gfx::Rect) {
                    .x = 20,
                    .y = 180,
                    .d = {
                        .w = 1200,
                        .h = 720,
                    },
                    .rotation = Gfx::GFX_ROTATION_270,
                },
                [1] = (Gfx::Rect) {
                    .x = 1260,
                    .y = 420,
                    .d = {
                        .w = 640,
                        .h = 480,
                    },
                    .rotation = Gfx::GFX_ROTATION_270,
                },
            },
        },

        .layout_drc = {
            [0] = (Gfx::Rect) {
                .x = 227,
                .y = 0,
                .d = {
                    .w = 400,
                    .h = 240,
                },
                .rotation = Gfx::GFX_ROTATION_270,
            },
            [1] = (Gfx::Rect) {
                .x = 267,
                .y = 240,
                .d = {
                    .w = 320,
                    .h = 240,
                },
                .rotation = Gfx::GFX_ROTATION_270,
            },
        }
    }};
};
