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

namespace Gfx {

static WHBGfxShaderGroup shader;
static int aPosition, aTexCoord;
static GX2UniformVar* uRCPScreenSize;

typedef struct _DrawCoords {
    struct {
        float x;
        float y;
    } coords[4];
} DrawCoords;

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

    if (!WHBGfxLoadGFDShaderGroup(&shader, 0, main_shader)) {
//#ifndef NDEBUG
        printf("[GX2] Couldn't parse shader!\n");
//#endif
        return false;
    }

    int buffer = 0;
    aPosition = buffer++;
    WHBGfxInitShaderAttribute(&shader, "aPosition", aPosition, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32);
    aTexCoord = buffer++;
    WHBGfxInitShaderAttribute(&shader, "aTexCoord", aTexCoord, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32);

    uRCPScreenSize = GX2GetVertexUniformVar(shader.vertexShader, "uRCPScreenSize");
    if (!uRCPScreenSize) {
//#ifndef NDEBUG
        printf("[GX2] Couldn't find uRCPScreenSize!\n");
//#endif
        return false;
    }

    WHBGfxInitFetchShader(&shader);

    return true;
}

Texture::Texture(int w, int h) {
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

    GX2RCreateBuffer(&aPositionBufferTV);
    GX2RCreateBuffer(&aTexCoordBufferTV);
    GX2RCreateBuffer(&aPositionBufferDRC);
    GX2RCreateBuffer(&aTexCoordBufferDRC);
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

    GX2SetFetchShader(&shader.fetchShader);
    GX2SetVertexShader(shader.vertexShader);
    GX2SetPixelShader(shader.pixelShader);

    GX2SetPixelSampler(&this->sampler, 0);
    GX2SetPixelTexture(&this->gx2_tex, 0);

    //Shader needs the reciprocal of the screen size to fix coordinates
    GX2SetVertexUniformReg(uRCPScreenSize->offset, 2, (void*)&curRCPScreenSize);

    GX2RBuffer* aPositionBuffer = (isRenderingDRC) ? &aPositionBufferDRC : &aPositionBufferTV;
    GX2RBuffer* aTexCoordBuffer = (isRenderingDRC) ? &aTexCoordBufferDRC : &aTexCoordBufferTV;

    DrawCoords* aPositions = (DrawCoords*)GX2RLockBufferEx(
        aPositionBuffer, (GX2RResourceFlags)0
    );
    if (!aPositions) return;
    //screen coordinates are fixed up to the usual -1.0f:1.0f in the shader
    *aPositions = (DrawCoords) { .coords = {
        [0] = { .x = (float)dest.x,            .y = (float)dest.y            },
        [1] = { .x = (float)dest.x + dest.d.w, .y = (float)dest.y            },
        [2] = { .x = (float)dest.x + dest.d.w, .y = (float)dest.y + dest.d.h },
        [3] = { .x = (float)dest.x,            .y = (float)dest.y + dest.d.h },
    }};
    GX2RUnlockBufferEx(aPositionBuffer, (GX2RResourceFlags)0);

    DrawCoords* aTexCoords = (DrawCoords*)GX2RLockBufferEx(
        aTexCoordBuffer, (GX2RResourceFlags)0
    );
    if (!aTexCoords) return;
    *aTexCoords = lookup_tex_coords[dest.rotation];
    GX2RUnlockBufferEx(aTexCoordBuffer, (GX2RResourceFlags)0);

    GX2RSetAttributeBuffer(
        aPositionBuffer, aPosition, aPositionBuffer->elemSize, 0
    );
    GX2RSetAttributeBuffer(
        aTexCoordBuffer, aTexCoord, aTexCoordBuffer->elemSize, 0
    );

    GX2DrawEx(GX2_PRIMITIVE_MODE_QUADS, aPositionBuffer->elemCount, 0, 1);
}

void Texture::RenderText(std::string text) {
    #warning TODO
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

//number cache
std::unordered_map<char, Texture> numbers_cache;

std::optional<std::reference_wrapper<Texture>> GetCachedNumber(char num) {
    if (!numbers_cache.contains(num)) {
        return std::nullopt;
    }

    return std::optional<std::reference_wrapper<Texture>>(numbers_cache[num]);
}
void CacheNumber(char num) {
    auto num_tex = numbers_cache.try_emplace(num);
    auto& tex = num_tex.first->second;
    if (!num_tex.second && tex.valid()) return;

    std::string str(&num, 1);
    tex.RenderText(str);
}

} //namespace Gfx
