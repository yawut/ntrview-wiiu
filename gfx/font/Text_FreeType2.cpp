#include "Text.hpp"
#include "gfx/Gfx.hpp"
#include "common.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include <unordered_map>

namespace Text {

const static int PIXEL_HEIGHT = 48;

static FT_Library library;
static FT_Face opensans;

static std::unordered_map<FT_UInt, std::pair<Gfx::Rect, Gfx::Texture>> glyph_cache;

static bool cacheNewGlyph(FT_UInt glyph_index) {
    auto error = FT_Load_Glyph(opensans, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_RENDER);
    if (error) {
        printf("[FT2] Couldn't render glyph %d, err %d\n", glyph_index, error);
        return false;
    }

    auto map_tex = glyph_cache.try_emplace(
        glyph_index,
        std::piecewise_construct,
        std::forward_as_tuple((Gfx::Rect) {
            .x =  opensans->glyph->bitmap_left, //x
            .y = -opensans->glyph->bitmap_top, //y
            .d = {
                .w = opensans->glyph->advance.x >> 6, //w
                .h = opensans->glyph->advance.y >> 6, //h
            },
        }),
        std::forward_as_tuple(
            opensans->glyph->bitmap.width,
            opensans->glyph->bitmap.rows,
            Gfx::DRAWMODE_TEXT
        )
    );
    //did we make a new texture?
    if (!map_tex.second) {
        printf("[FT2] BUG: re-adding existing glyph %d?\n", glyph_index);
        return false;
    }
    //lol
    Gfx::Texture& tex = map_tex.first->second.second;

    FT_Bitmap& bmp = opensans->glyph->bitmap;
    if (bmp.pixel_mode != FT_PIXEL_MODE_GRAY) {
        printf("[FT2] Unsupported pixel mode %d!\n", bmp.pixel_mode);
        return false;
    }
    if (bmp.pitch <= 0) {
        printf("[FT2] Unsupported pixel pitch %d! (%dx%d, %d)\n", bmp.pitch, bmp.width, bmp.rows, glyph_index);
        return false;
    }
    auto pixels = tex.Lock();
    if (!tex.locked || pixels.empty()) {
        printf("[FT2] BUG: Couldn't lock texture for render!\n");
        return false;
    }

    //sadly FT_Bitmap_Convert likes to reallocate buffers, so we do this by hand
    for (uint y = 0; y < bmp.rows; y++) {
        for (uint x = 0; x < bmp.width; x++) {
            uint bmp_offset = (y * bmp.pitch) + x;
            uint tex_offset = (y * tex.pitch) + x;
            if (tex_offset >= pixels.size()) {
                printf("[FT2] BUG: OOB buffer copy on tex!\n");
                break;
            }
            pixels[tex_offset] = bmp.buffer[bmp_offset];
        }
    }

    tex.Unlock(pixels);
    return true;
}

void Text::Render(int x, int y) {
    y += PIXEL_HEIGHT; //fix for SDL2_ttf compat
    for (auto c : this->text) {
        FT_UInt glyph_index = FT_Get_Char_Index(opensans, c);
        if (!glyph_cache.contains(glyph_index)) {
            bool ok = cacheNewGlyph(glyph_index);
            if (!ok) {
                continue;
            }
        }

        auto& tex_pair = glyph_cache[glyph_index];
        Gfx::Rect& tex_rect = tex_pair.first;
        Gfx::Texture& tex = tex_pair.second;
        /*printf("\"%s\": %c @ (%d,%d):%dx%d ((%d,%d):%dx%d)\n",
            text.c_str(), c, x + tex_rect.x, y + tex_rect.y,
            tex_rect.d.w, tex_rect.d.h, x, y, tex.d.w, tex.d.h
        );*/
        tex.Render((Gfx::Rect) {
            .x = x + tex_rect.x,
            .y = y + tex_rect.y,
            .d = tex.d,
        });
        x += tex_rect.d.w;
        //TODO FT2 height adjustments
    }
}

Text::Text(std::string text) :
    text(text) {

    this->d.w = 0;
    for (auto c : this->text) {
        FT_UInt glyph_index = FT_Get_Char_Index(opensans, c);
        if (!glyph_cache.contains(glyph_index)) {
            //printf("new: %c:%d\n", c, glyph_index);
            bool ok = cacheNewGlyph(glyph_index);
            if (!ok) {
                continue;
            }
        }

        this->d.w += glyph_cache[glyph_index].first.d.w;
    }

    this->baseline_y = 24; //uhh
    this->d.h = PIXEL_HEIGHT + this->baseline_y; //not strictly true
    //printf("\"%s\" is %dx%d\n", this->text.c_str(), this->d.w, this->d.h);
}

bool Init() {
    auto error = FT_Init_FreeType(&library);
    if (error) {
        printf("[FT2] Init error %d!\n", error);
        return false;
    }

    error = FT_New_Face(library, RAMFS_DIR "/opensans.ttf", 0, &opensans);
    if (error) {
        printf("[FT2] Couldn't read font! %d\n", error);
        return false;
    }

    error = FT_Set_Pixel_Sizes(opensans, 0, PIXEL_HEIGHT);
    if (error) {
        printf("[FT2] Couldn't set font size! %d\n", error);
        return false;
    }

    return true;
}

void Quit() {
    #warning TODO
}

} //namespace FontFT2
