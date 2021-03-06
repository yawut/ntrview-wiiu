#pragma once
#include <array>

//it's not cheating if it's fast

static DrawCoords lookup_tex_coords[4] = {
    [GFX_ROTATION_0] = { .coords = {
        [0] = { .x =  0.0f, .y =  0.0f },
        [1] = { .x =  1.0f, .y =  0.0f },
        [2] = { .x =  1.0f, .y =  1.0f },
        [3] = { .x =  0.0f, .y =  1.0f },
    }},
/*  I feel like this and GFX_ROTATION_270 are backwards to what they should be
    However, after much head-scratching, I couldn't come up with a good reason
    It's probably something to do with the y-axis inversion the shader does */
    [GFX_ROTATION_90] = { .coords = {
        [0] = { .x =  0.0f, .y =  1.0f },
        [1] = { .x =  0.0f, .y =  0.0f },
        [2] = { .x =  1.0f, .y =  0.0f },
        [3] = { .x =  1.0f, .y =  1.0f },
    }},
    [GFX_ROTATION_180] = { .coords = {
        [0] = { .x =  1.0f, .y =  1.0f },
        [1] = { .x =  0.0f, .y =  1.0f },
        [2] = { .x =  0.0f, .y =  0.0f },
        [3] = { .x =  1.0f, .y =  0.0f },
    }},
    [GFX_ROTATION_270] = { .coords = {
        [0] = { .x =  1.0f, .y =  0.0f },
        [1] = { .x =  1.0f, .y =  1.0f },
        [2] = { .x =  0.0f, .y =  1.0f },
        [3] = { .x =  0.0f, .y =  0.0f },
    }},
};
