#pragma once

//how much C++ can a C programmer squeeze into one header? well...

#include <memory>
#include <span>

#ifdef GFX_SDL
#include <SDL.h>
#include <SDL_ttf.h>
#endif

namespace Gfx {

typedef struct Dimensions {
    int w;
    int h;
} Dimensions;
typedef struct Rect {
    int x;
    int y;
    Dimensions d;
    int angle = 0;
} Rect;
typedef struct rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a = 0xFF;
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
#endif
};

bool Init();
void Quit();
void Clear(rgb colour);
void Present();

std::optional<std::reference_wrapper<Texture>> GetCachedNumber(char num);
void CacheNumber(char num);

const char* GetError();

} //namespace Gfx
