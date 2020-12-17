#include "Gfx.hpp"
#include "common.h"

#include <unordered_map>

#include <gx2_shaders.h>
#include <whb/gfx.h>
#include <gx2r/buffer.h>
#include <gx2r/draw.h>
#include <gx2/shaders.h>
#include <gx2/draw.h>
#include <gx2/registers.h>
#include <gx2/mem.h>

namespace Gfx {

static struct {
    WHBGfxShaderGroup shader;
    int aPosition, aTexCoord;
    GX2UniformVar* uRCPScreenSize;
} shader_main;
static struct {
    WHBGfxShaderGroup shader;
    int aPosition, aTexCoord;
    GX2UniformVar* uRCPScreenSize;
} shader_text;
static struct {
    WHBGfxShaderGroup shader;
    int aPosition, aColour;
    GX2UniformVar* uRCPScreenSize;
} shader_colour;

#define NUM_VERTEXES 4
typedef struct alignas(GX2_VERTEX_BUFFER_ALIGNMENT) _DrawCoords {
    struct {
        float x;
        float y;
    } coords[NUM_VERTEXES];
} DrawCoords;
std::vector<DrawCoords> vertex_cache;

struct _DrawColour {
    float r;
    float g;
    float b;
    float a;
};
typedef struct alignas(GX2_VERTEX_BUFFER_ALIGNMENT) _DrawColours {
    struct _DrawColour colours[NUM_VERTEXES];
} DrawColours;
static inline struct _DrawColour mkDrawColour(const rgb& c) {
    return (struct _DrawColour){
        .r = c.flt_r(), .g = c.flt_g(), .b = c.flt_b(), .a = c.flt_a()
    };
}
std::vector<DrawColours> colour_cache;

typedef struct _ScreenSize {
    float w;
    float h;
} ScreenSize;
static ScreenSize curRCPScreenSize;

bool isRenderingDRC = false;

#include "Gfx_GX2_lookup_tables.inc"

void Quit() {
    WHBGfxShutdown();
}

bool Init() {
    WHBGfxInit();

    //init main shader
    if (!WHBGfxLoadGFDShaderGroup(&shader_main.shader, 0, main_shader)) {
        printf("[GX2] Couldn't parse shader!\n");
        return false;
    }

    int buffer = 0;
    shader_main.aPosition = buffer++;
    WHBGfxInitShaderAttribute(&shader_main.shader, "aPosition", shader_main.aPosition, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32);
    shader_main.aTexCoord = buffer++;
    WHBGfxInitShaderAttribute(&shader_main.shader, "aTexCoord", shader_main.aTexCoord, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32);

    shader_main.uRCPScreenSize = GX2GetVertexUniformVar(shader_main.shader.vertexShader, "uRCPScreenSize");
    if (!shader_main.uRCPScreenSize) {
        printf("[GX2] Couldn't find uRCPScreenSize!\n");
        return false;
    }

    WHBGfxInitFetchShader(&shader_main.shader);

    //init text shader
    if (!WHBGfxLoadGFDShaderGroup(&shader_text.shader, 0, text_shader)) {
        printf("[GX2] Couldn't parse shader!\n");
        return false;
    }

    buffer = 0;
    shader_text.aPosition = buffer++;
    WHBGfxInitShaderAttribute(&shader_text.shader, "aPosition", shader_text.aPosition, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32);
    shader_text.aTexCoord = buffer++;
    WHBGfxInitShaderAttribute(&shader_text.shader, "aTexCoord", shader_text.aTexCoord, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32);

    shader_text.uRCPScreenSize = GX2GetVertexUniformVar(shader_text.shader.vertexShader, "uRCPScreenSize");
    if (!shader_text.uRCPScreenSize) {
        printf("[GX2] Couldn't find uRCPScreenSize!\n");
        return false;
    }

    WHBGfxInitFetchShader(&shader_text.shader);

    //init colour shader
    if (!WHBGfxLoadGFDShaderGroup(&shader_colour.shader, 0, colour_shader)) {
        printf("[GX2] Couldn't parse shader!\n");
        return false;
    }

    buffer = 0;
    shader_colour.aPosition = buffer++;
    WHBGfxInitShaderAttribute(&shader_colour.shader, "aPosition", shader_colour.aPosition, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32);
    shader_colour.aColour = buffer++;
    WHBGfxInitShaderAttribute(&shader_colour.shader, "aColour", shader_colour.aColour, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32_32_32);

    shader_colour.uRCPScreenSize = GX2GetVertexUniformVar(shader_colour.shader.vertexShader, "uRCPScreenSize");
    if (!shader_colour.uRCPScreenSize) {
        printf("[GX2] Couldn't find uRCPScreenSize!\n");
        return false;
    }

    WHBGfxInitFetchShader(&shader_colour.shader);

    return true;
}

Texture::Texture(int w, int h, DrawMode mode) :
    mode(mode) {

    switch (mode) {
        case DRAWMODE_TEXTURE_RGB: {
            this->gx2_tex.surface.format = GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
            this->bpp = 32;
            this->bypp = 4;
            break;
        }
        case DRAWMODE_TEXT: {
            this->gx2_tex.surface.format = GX2_SURFACE_FORMAT_UNORM_R8;
            this->bpp = 8;
            this->bypp = 1;
            break;
        }
    }

    GX2InitSampler(&this->sampler,
        GX2_TEX_CLAMP_MODE_WRAP, GX2_TEX_XY_FILTER_MODE_POINT
    );

    this->d.w = w;
    this->d.h = h;
    this->gx2_tex.surface.width  = w;
    this->gx2_tex.surface.height = h;
    GX2RCreateSurface(&this->gx2_tex.surface,
        (GX2RResourceFlags)
            (GX2R_RESOURCE_BIND_TEXTURE |
             GX2R_RESOURCE_USAGE_CPU_WRITE |
             GX2R_RESOURCE_USAGE_GPU_READ)
    );
    GX2InitTextureRegs(&this->gx2_tex);
    this->pitch = this->gx2_tex.surface.pitch * this->bypp;
}

std::span<uint8_t> Texture::Lock() {
    uint8_t* surface = (uint8_t*)GX2RLockSurfaceEx(&this->gx2_tex.surface, 0, (GX2RResourceFlags)0);
    if (!surface) {
        return std::span<uint8_t>();
    }
    this->locked = true;
    return std::span<uint8_t>(surface, this->gx2_tex.surface.imageSize);
}
void Texture::Unlock(std::span<uint8_t>& pixels) {
    GX2RUnlockSurfaceEx(&this->gx2_tex.surface, 0, (GX2RResourceFlags)0);
    pixels = std::span<uint8_t>();
    this->locked = false;
}

void Texture::Render(Rect dest) {
    GX2SetDepthOnlyControl(FALSE, FALSE, GX2_COMPARE_FUNC_ALWAYS);
    GX2SetColorControl(GX2_LOGIC_OP_COPY, 0xFF, FALSE, TRUE);
    GX2SetBlendControl(GX2_RENDER_TARGET_0,
        /* RGB = [srcRGB * srcA] + [dstRGB * (1-srcA)] */
        GX2_BLEND_MODE_SRC_ALPHA, GX2_BLEND_MODE_INV_SRC_ALPHA,
        GX2_BLEND_COMBINE_MODE_ADD,
        TRUE,
        /* A = [srcA * 1] + [dstA * (1-srcA)] */
        GX2_BLEND_MODE_ONE, GX2_BLEND_MODE_INV_SRC_ALPHA,
        GX2_BLEND_COMBINE_MODE_ADD
    );

    GX2SetPixelSampler(&this->sampler, 0);
    GX2SetPixelTexture(&this->gx2_tex, 0);

    DrawCoords& aPositions = vertex_cache.emplace_back((DrawCoords) {.coords={
        [0] = { .x = (float)dest.x,            .y = (float)dest.y            },
        [1] = { .x = (float)dest.x + dest.d.w, .y = (float)dest.y            },
        [2] = { .x = (float)dest.x + dest.d.w, .y = (float)dest.y + dest.d.h },
        [3] = { .x = (float)dest.x,            .y = (float)dest.y + dest.d.h },
    }});
    //this could probably be indexed for a bit more speed
    DrawCoords& aTexCoords = vertex_cache.emplace_back(lookup_tex_coords[dest.rotation]);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_ATTRIBUTE_BUFFER, &aPositions, sizeof(aPositions));
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_ATTRIBUTE_BUFFER, &aTexCoords, sizeof(aTexCoords));

    switch (this->mode) {
        case DRAWMODE_TEXTURE_RGB: {
            GX2SetFetchShader(&shader_main.shader.fetchShader);
            GX2SetVertexShader(shader_main.shader.vertexShader);
            GX2SetPixelShader(shader_main.shader.pixelShader);

            GX2SetAttribBuffer(
                shader_main.aPosition,
                sizeof(aPositions),
                sizeof(aPositions.coords[0]),
                &aPositions
            );
            GX2SetAttribBuffer(
                shader_main.aTexCoord,
                sizeof(aTexCoords),
                sizeof(aTexCoords.coords[0]),
                &aTexCoords
            );

            //Shader needs the reciprocal of the screen size to fix coordinates
            GX2SetVertexUniformReg(shader_main.uRCPScreenSize->offset, 2, (void*)&curRCPScreenSize);
            break;
        }
        case DRAWMODE_TEXT: {
            GX2SetFetchShader(&shader_text.shader.fetchShader);
            GX2SetVertexShader(shader_text.shader.vertexShader);
            GX2SetPixelShader(shader_text.shader.pixelShader);

            GX2SetAttribBuffer(
                shader_text.aPosition,
                sizeof(aPositions),
                sizeof(aPositions.coords[0]),
                &aPositions
            );
            GX2SetAttribBuffer(
                shader_text.aTexCoord,
                sizeof(aTexCoords),
                sizeof(aTexCoords.coords[0]),
                &aTexCoords
            );

            //Shader needs the reciprocal of the screen size to fix coordinates
            GX2SetVertexUniformReg(shader_text.uRCPScreenSize->offset, 2, (void*)&curRCPScreenSize);
            break;
        }
    }

    GX2DrawEx(GX2_PRIMITIVE_MODE_QUADS, NUM_VERTEXES, 0, 1);
}

void DrawFillRect(const FillRect& rect) {
    GX2SetDepthOnlyControl(FALSE, FALSE, GX2_COMPARE_FUNC_ALWAYS);
    GX2SetColorControl(GX2_LOGIC_OP_COPY, 0xFF, FALSE, TRUE);
    GX2SetBlendControl(GX2_RENDER_TARGET_0,
        /* RGB = [srcRGB * srcA] + [dstRGB * (1-srcA)] */
        GX2_BLEND_MODE_SRC_ALPHA, GX2_BLEND_MODE_INV_SRC_ALPHA,
        GX2_BLEND_COMBINE_MODE_ADD,
        TRUE,
        /* A = [srcA * 1] + [dstA * (1-srcA)] */
        GX2_BLEND_MODE_ONE, GX2_BLEND_MODE_INV_SRC_ALPHA,
        GX2_BLEND_COMBINE_MODE_ADD
    );

    auto& dest = rect.r;
    DrawCoords& aPositions = vertex_cache.emplace_back((DrawCoords) {.coords={
        [0] = { .x = (float)dest.x,            .y = (float)dest.y            },
        [1] = { .x = (float)dest.x + dest.d.w, .y = (float)dest.y            },
        [2] = { .x = (float)dest.x + dest.d.w, .y = (float)dest.y + dest.d.h },
        [3] = { .x = (float)dest.x,            .y = (float)dest.y + dest.d.h },
    }});
    DrawColours& aColours = colour_cache.emplace_back((DrawColours) {.colours={
        [0] = mkDrawColour(rect.c[0]),
        [1] = mkDrawColour(rect.c[1]),
        [2] = mkDrawColour(rect.c[2]),
        [3] = mkDrawColour(rect.c[3]),
    }});
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_ATTRIBUTE_BUFFER, &aPositions, sizeof(aPositions));
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_ATTRIBUTE_BUFFER, &aColours, sizeof(aColours));

    GX2SetFetchShader(&shader_colour.shader.fetchShader);
    GX2SetVertexShader(shader_colour.shader.vertexShader);
    GX2SetPixelShader(shader_colour.shader.pixelShader);

    GX2SetAttribBuffer(
        shader_colour.aPosition,
        sizeof(aPositions),
        sizeof(aPositions.coords[0]),
        &aPositions
    );
    GX2SetAttribBuffer(
        shader_colour.aColour,
        sizeof(aColours),
        sizeof(aColours.colours[0]),
        &aColours
    );

    //Shader needs the reciprocal of the screen size to fix coordinates
    GX2SetVertexUniformReg(shader_colour.uRCPScreenSize->offset, 2, (void*)&curRCPScreenSize);

    GX2DrawEx(GX2_PRIMITIVE_MODE_QUADS, NUM_VERTEXES, 0, 1);
}

void Clear(rgb colour) {
    WHBGfxClearColor(colour.flt_r(), colour.flt_g(), colour.flt_b(), colour.flt_a());
}

void PrepRender() {
    WHBGfxBeginRender();
}
void PrepRenderTop() {
    WHBGfxBeginRenderTV();
    GX2ColorBuffer* tvCbuf = WHBGfxGetTVColourBuffer();
    curRCPScreenSize = (ScreenSize) {
        .w = 1.0f / (float)tvCbuf->surface.width,
        .h = 1.0f / (float)tvCbuf->surface.height,
    };
    isRenderingDRC = false;
}
void DoneRenderTop() {
    WHBGfxFinishRenderTV();
}
void PrepRenderBtm() {
    WHBGfxBeginRenderDRC();
    GX2ColorBuffer* drcCbuf = WHBGfxGetDRCColourBuffer();
    curRCPScreenSize = (ScreenSize) {
        .w = 1.0f / (float)drcCbuf->surface.width,
        .h = 1.0f / (float)drcCbuf->surface.height,
    };
    isRenderingDRC = true;
}
void DoneRenderBtm() {
    WHBGfxFinishRenderDRC();
}
void Present() {
    WHBGfxFinishRender();
    vertex_cache.clear();
}

const char* GetError() {
    return "GX2 doesn't get errors, thanks";
}

Resolution GetResolution() {
    GX2ColorBuffer* tvCbuf = WHBGfxGetTVColourBuffer();
    switch (tvCbuf->surface.height) {
        case 480: return RESOLUTION_480P;
        case 720: return RESOLUTION_720P;
        case 1080: return RESOLUTION_1080P;
        default: {
            printf("[GX2] Running at unknown resolution %dp?\n", tvCbuf->surface.height);
            return RESOLUTION_720P;
        }
    }
}

} //namespace Gfx
