#include "Gfx.hpp"
#include "common.h"

#include <unordered_map>

#include <SDL.h>
#include <SDL_ttf.h>

namespace Gfx {

static SDL_Window* window;
static SDL_Renderer* renderer;
static TTF_Font* font;

void Quit() {
    SDL_Quit();
}

bool Init() {
    int ret;

    /*  Init libraries */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("[SDL] Couldn't init SDL!\n");
        Quit();
        return false;
    }

    if (SDL_CreateWindowAndRenderer(1280, 720, 0, &window, &renderer)) {
        printf("[SDL] Couldn't make window and renderer: %s\n", SDL_GetError());
        Quit();
        return false;
    }

#ifndef NDEBUG
    printf("[SDL] Initialised graphics\n");
#endif

    ret = TTF_Init();
    if (ret < 0) {
        printf("[SDL] Couldn't initialise fonts: %s\n", TTF_GetError());
        Quit();
        return false;
    }

    font = TTF_OpenFont(RAMFS_DIR "/opensans.ttf", 64);
    if (!font) {
        printf("[SDL] Couldn't open font: %s\n", TTF_GetError());
        Quit();
        return false;
    }

    return true;
}

Texture::Texture(int w, int h) {
    this->sdl_tex = SDL_CreateTexture(renderer,
        IS_BIG_ENDIAN ? SDL_PIXELFORMAT_RGBA8888 : SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING,
        w, h
    );
    Uint32 format;
    int access;
    SDL_QueryTexture(this->sdl_tex, &format, &access, &this->d.w, &this->d.h);
}

std::span<uint8_t> Texture::Lock() {
    void* pixels;
    int ret = SDL_LockTexture(this->sdl_tex, NULL, &pixels, &this->pitch);
    if (ret == 0) {
        this->locked = true;
        return std::span<uint8_t>((uint8_t*)pixels, this->d.w * this->d.h * this->bypp);
    }
    return std::span<uint8_t>();
}
void Texture::Unlock(std::span<uint8_t>& pixels) {
    SDL_UnlockTexture(this->sdl_tex);
    pixels = std::span<uint8_t>();
    this->locked = false;
}

void Texture::Render(Rect dest) {
    SDL_Rect dstRect = {
        .x = dest.x,
        .y = dest.y,
        .w = dest.d.w,
        .h = dest.d.h,
    };
    int angle = dest.rotation * 90;
    int ret = SDL_RenderCopyEx(renderer, this->sdl_tex, NULL, &dstRect, angle, NULL, SDL_FLIP_NONE);
    if (ret) {
        printf("[SDL] %s\n", SDL_GetError());
    }
}

void Texture::RenderText(std::string text) {
    SDL_Surface* text_surf = TTF_RenderUTF8_Blended(font,
        text.c_str(),
        (SDL_Color) { 0xFF, 0xFF, 0xFF }
    );
    this->sdl_tex = SDL_CreateTextureFromSurface(renderer, text_surf);
    SDL_FreeSurface(text_surf);
    text_surf = NULL;

    Uint32 format;
    int access;
    SDL_QueryTexture(this->sdl_tex, &format, &access, &this->d.w, &this->d.h);
}

void Clear(rgb colour) {
    SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, colour.a);
    SDL_RenderClear(renderer);
}

void PrepRender() {};
void PrepRenderTop() {};
void DoneRenderTop() {};
void PrepRenderBtm() {};
void DoneRenderBtm() {};

void Present() {
    SDL_RenderPresent(renderer);
}

const char* GetError() {
    return SDL_GetError();
}

Resolution GetResolution() {
    return RESOLUTION_720P;
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
