#pragma once

//how much C++ can a C programmer squeeze into one header? well...

#include <memory>
#include <span>

#ifdef GFX_SDL
#include <SDL.h>
#include <SDL_ttf.h>
#elif defined(GFX_GX2)
#include <gx2r/buffer.h>
#include <gx2r/surface.h>
#include <gx2/texture.h>
#include <gx2/utils.h>
#include <gx2/sampler.h>
#endif

namespace Gfx {

typedef enum Rotation {
    GFX_ROTATION_0 = 0,
    GFX_ROTATION_90 = 1,
    GFX_ROTATION_180 = 2,
    GFX_ROTATION_270 = 3,
} Rotation;
typedef enum Resolution {
    RESOLUTION_480P  = 0,
    RESOLUTION_720P  = 1,
    RESOLUTION_1080P = 2,

    RESOLUTION_MAX = 3,
} Resolution;

typedef struct Dimensions {
    int w;
    int h;
} Dimensions;
typedef struct Rect {
    int x;
    int y;
    Dimensions d;
    Rotation rotation = GFX_ROTATION_0;
} Rect;
typedef struct rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a = 0xFF;
    float flt_r() { return (float)r / 255; }
    float flt_g() { return (float)g / 255; }
    float flt_b() { return (float)b / 255; }
    float flt_a() { return (float)a / 255; }
} rgb;

class Texture {
public:
    Dimensions d;
    const int bpp = 32;
    const int bypp = 4;
    void Render(Rect dest);

    bool locked = false;
    int pitch;
    std::span<uint8_t> Lock();
    void Unlock(std::span<uint8_t>& pixels);

    Texture(int w, int h);
    Texture() {};

    void RenderText(std::string text);
    Texture(std::string text) { RenderText(text); }

#if defined(GFX_SDL)
    SDL_Texture* sdl_tex = nullptr;
    bool valid() { return sdl_tex != nullptr; }
#elif defined(GFX_GX2)
    GX2RBuffer aPositionBufferTV = {
        .flags = (GX2RResourceFlags) (
            GX2R_RESOURCE_BIND_VERTEX_BUFFER | GX2R_RESOURCE_USAGE_CPU_WRITE
        ),
        .elemSize = sizeof(float) * 2,
        .elemCount = 4,
    };
    GX2RBuffer aTexCoordBufferTV = {
        .flags = (GX2RResourceFlags) (
            GX2R_RESOURCE_BIND_VERTEX_BUFFER | GX2R_RESOURCE_USAGE_CPU_WRITE
        ),
        .elemSize = sizeof(float) * 2,
        .elemCount = 4,
    };
    GX2RBuffer aPositionBufferDRC = {
        .flags = (GX2RResourceFlags) (
            GX2R_RESOURCE_BIND_VERTEX_BUFFER | GX2R_RESOURCE_USAGE_CPU_WRITE
        ),
        .elemSize = sizeof(float) * 2,
        .elemCount = 4,
    };
    GX2RBuffer aTexCoordBufferDRC = {
        .flags = (GX2RResourceFlags) (
            GX2R_RESOURCE_BIND_VERTEX_BUFFER | GX2R_RESOURCE_USAGE_CPU_WRITE
        ),
        .elemSize = sizeof(float) * 2,
        .elemCount = 4,
    };
    GX2Sampler sampler;
    GX2Texture gx2_tex = {
        .surface = (GX2Surface) {
            .dim = GX2_SURFACE_DIM_TEXTURE_2D,
            .depth = 1,
            .mipLevels = 1,
            .format = GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8,
            .tileMode = GX2_TILE_MODE_LINEAR_ALIGNED,
        },
        .viewNumSlices = 1,
        .compMap = GX2_COMP_MAP(
            GX2_SQ_SEL_R, GX2_SQ_SEL_G, GX2_SQ_SEL_B, GX2_SQ_SEL_A
        ),
    };
    bool valid() { return true; }
#endif
};

bool Init();
void Quit();
void Clear(rgb colour);
void PrepRender();
void PrepRenderTop();
void DoneRenderTop();
void PrepRenderBtm();
void DoneRenderBtm();
void Present();

std::optional<std::reference_wrapper<Texture>> GetCachedNumber(char num);
void CacheNumber(char num);

const char* GetError();
Resolution GetResolution();

} //namespace Gfx
