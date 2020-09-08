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

typedef struct _DrawCoords {
    struct {
        float x;
        float y;
    } coords[4];
} DrawCoords;

void Quit() {
    WHBGfxShutdown();
}

bool Init() {
    WHBGfxInit();

    if (!WHBGfxLoadGFDShaderGroup(&shader, 0, main_shader)) {
        printf("[GX2] Couldn't parse shader!\n");
        Quit();
        return false;
    }

    int buffer = 0;
    aPosition = buffer++;
    WHBGfxInitShaderAttribute(&shader, "aPosition", aPosition, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32);
    aTexCoord = buffer++;
    WHBGfxInitShaderAttribute(&shader, "aTexCoord", aTexCoord, 0, GX2_ATTRIB_FORMAT_FLOAT_32_32);

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

    GX2RCreateBuffer(&aPositionBuffer);
    DrawCoords* aPositions = (DrawCoords*)GX2RLockBufferEx(
        &aPositionBuffer, (GX2RResourceFlags)0
    );
    *aPositions = (DrawCoords) { .coords = {
        [0] = { .x = -1.0f, .y =  1.0f },
        [1] = { .x =  1.0f, .y =  1.0f },
        [2] = { .x =  1.0f, .y = -1.0f },
        [3] = { .x = -1.0f, .y = -1.0f },
    }};
    GX2RUnlockBufferEx(&aPositionBuffer, (GX2RResourceFlags)0);

    GX2RCreateBuffer(&aTexCoordBuffer);
    DrawCoords* aTexCoords = (DrawCoords*)GX2RLockBufferEx(
        &aTexCoordBuffer, (GX2RResourceFlags)0
    );
    *aTexCoords = (DrawCoords) { .coords = {
        [0] = { .x =  0.0f, .y =  0.0f },
        [1] = { .x =  1.0f, .y =  0.0f },
        [2] = { .x =  1.0f, .y =  1.0f },
        [3] = { .x =  0.0f, .y =  1.0f },
    }};
    GX2RUnlockBufferEx(&aTexCoordBuffer, (GX2RResourceFlags)0);
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

    GX2RSetAttributeBuffer(
        &aPositionBuffer, aPosition, aPositionBuffer.elemSize, 0
    );
    GX2RSetAttributeBuffer(
        &aTexCoordBuffer, aTexCoord, aTexCoordBuffer.elemSize, 0
    );

    GX2DrawEx(GX2_PRIMITIVE_MODE_QUADS, aPositionBuffer.elemCount, 0, 1);
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
}
void DoneRenderTop() {
    WHBGfxFinishRenderTV();
}
void PrepRenderBtm() {
    WHBGfxBeginRenderDRC();
}
void DoneRenderBtm() {
    WHBGfxFinishRenderDRC();
}
void Present() {
    WHBGfxFinishRender();
}

const char* GetError() {
    #warning TODO
    return "Not there yet";
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
