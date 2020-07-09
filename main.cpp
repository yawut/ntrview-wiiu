#include <SDL.h>
#include <SDL_ttf.h>
#include <turbojpeg.h>
#include <INIReader.h>


#ifdef __WIIU__
#include <whb/log.h>
#include <whb/log_udp.h>
#include <whb/log_cafe.h>
#include <whb/proc.h>
#include <sys/iosupport.h>
#include <romfs-wiiu.h>
#define USE_RAMFS

#define NTRVIEW_DIR "fs:/vol/external01/wiiu/apps/ntrview"
#define RAMFS_DIR "resin:/res"

static ssize_t wiiu_log_write(struct _reent* r, void* fd, const char* ptr, size_t len) {
    (void)r; (void)fd;
    WHBLogPrintf("%*.*s", len, len, ptr);
    return len;
}
static devoptab_t dotab_stdout = {
    .name = "udp_out",
    .write_r = &wiiu_log_write,
};
#else
#include <stdio.h>

#define NTRVIEW_DIR "."
#define RAMFS_DIR "../resin/res"

#endif

#define IS_BIG_ENDIAN (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)

#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <thread>

#include "Network.hpp"

std::map<char, SDL_Texture*> numbers_text = {
    {'0', NULL},
    {'1', NULL},
    {'2', NULL},
    {'3', NULL},
    {'4', NULL},
    {'5', NULL},
    {'6', NULL},
    {'7', NULL},
    {'8', NULL},
    {'9', NULL},
    {':', NULL},
    {'.', NULL}
};

int main(int argc, char** argv) {
    Uint32 format;
    int access, ret;
    (void)argc, (void)argv;

    #ifdef USE_RAMFS
    ramfsInit();
    #endif

    #ifdef __WIIU__
    WHBLogUdpInit();
    WHBLogCafeInit();
    devoptab_list[STD_OUT] = &dotab_stdout;
    devoptab_list[STD_ERR] = &dotab_stdout;
    #warning "Building for Wii U"
    #endif

    printf("hi\n");

/*  Init libraries */
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Couldn't init SDL!\n");
        SDL_Quit();
        return 3;
    }

    SDL_Window* window;
    SDL_Renderer* renderer;

    if (SDL_CreateWindowAndRenderer(1280, 720, 0, &window, &renderer)) {
        printf("Couldn't make window and renderer: %s\n", SDL_GetError());
        SDL_Quit();
        return 3;
    }

    printf("did init\n");

    ret = TTF_Init();
    if (ret < 0) {
        printf("Couldn't initialise fonts: %s\n", TTF_GetError());
        SDL_Quit();
        return 3;
    }

    TTF_Font* font = TTF_OpenFont(RAMFS_DIR "/opensans.ttf", 64);
    if (!font) {
        printf("Couldn't open font: %s\n", TTF_GetError());
        SDL_Quit();
        return 3;
    }

    tjhandle tj_handle = tjInitDecompress();

/*  Show loading text */
    SDL_Surface* loading_text_surf = TTF_RenderUTF8_Blended(font,
        "Now Loading",
        (SDL_Color) { 0xFF, 0xFF, 0xFF }
    );
    SDL_Texture* loading_text = SDL_CreateTextureFromSurface(renderer, loading_text_surf);
    SDL_FreeSurface(loading_text_surf);
    loading_text_surf = NULL;

    SDL_SetRenderDrawColor(renderer, 0x7F, 0x7F, 0x7F, 0xFF);
    SDL_RenderClear(renderer);

    SDL_Rect dstrect = { 0 };
    SDL_QueryTexture(loading_text, &format, &access, &dstrect.w, &dstrect.h);
    dstrect.y = 720 - dstrect.h;

    SDL_RenderCopy(renderer, loading_text, NULL, &dstrect);
    SDL_RenderPresent(renderer);

/*  Allocate textures for the received frames */
    SDL_Texture* topTexture = SDL_CreateTexture(renderer,
        IS_BIG_ENDIAN ? SDL_PIXELFORMAT_RGBA8888 : SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING,
        240, 400
    );
    if (!topTexture) {
        printf("Couldn't make texture: %s\n", SDL_GetError());
        SDL_Quit();
        return 3;
    }

    SDL_Texture* botTexture = SDL_CreateTexture(renderer,
        IS_BIG_ENDIAN ? SDL_PIXELFORMAT_RGBA8888 : SDL_PIXELFORMAT_ABGR8888,
        SDL_TEXTUREACCESS_STREAMING,
        240, 400
    );
    if (!botTexture) {
        printf("Couldn't make texture: %s\n", SDL_GetError());
        SDL_Quit();
        return 3;
    }

/*  Read config file */
#ifdef __WIIU__
    INIReader config(NTRVIEW_DIR "/ntrview.ini");
#else
    INIReader config("ntrview.ini");
#endif

    ret = config.ParseError();
    if (ret > 0) {
        printf("ntrview.ini syntax error on line %d!\n", ret);
    } else if (ret < 0) {
        printf("failed to open ntrview.ini\n");
    }

    std::string host       = config.Get("3ds", "ip", "10.0.0.10");
    uint8_t priority       = config.GetInteger("3ds", "priority", 1);
    uint8_t priorityFactor = config.GetInteger("3ds", "priorityFactor", 5);
    uint8_t jpegQuality    = config.GetInteger("3ds", "jpegQuality", 80);
    uint8_t QoS            = config.GetInteger("3ds", "QoS", 18);
    uint8_t bg_r = config.GetInteger("display", "background_r", 0x7F);
    uint8_t bg_g = config.GetInteger("display", "background_g", 0x7F);
    uint8_t bg_b = config.GetInteger("display", "background_b", 0x7F);

/*  Pre-render important texts */
    for (auto& pair : numbers_text) {
        char str[2] = { pair.first, '\0' };
        SDL_Surface* number_surf = TTF_RenderUTF8_Blended(font,
            str,
            (SDL_Color) { 0xFF, 0xFF, 0xFF }
        );
        pair.second = SDL_CreateTextureFromSurface(renderer, number_surf);
        SDL_FreeSurface(number_surf);
    }

    std::string connecting_text_str("Connecting to ");
    connecting_text_str.append(host);
    SDL_Surface* connecting_text_surf = TTF_RenderUTF8_Blended(font,
        connecting_text_str.c_str(),
        (SDL_Color) { 0xFF, 0xFF, 0xFF }
    );
    SDL_Texture* connecting_text = SDL_CreateTextureFromSurface(renderer, connecting_text_surf);
    SDL_FreeSurface(connecting_text_surf);
    connecting_text_surf = NULL;

    SDL_Surface* attempt_text_surf = TTF_RenderUTF8_Blended(font,
        ", attempt ",
        (SDL_Color) { 0xFF, 0xFF, 0xFF }
    );
    SDL_Texture* attempt_text = SDL_CreateTextureFromSurface(renderer, attempt_text_surf);
    SDL_FreeSurface(attempt_text_surf);
    attempt_text_surf = NULL;

    SDL_Surface* connected_text_surf = TTF_RenderUTF8_Blended(font,
        "Connected.",
        (SDL_Color) { 0xFF, 0xFF, 0xFF }
    );
    SDL_Texture* connected_text = SDL_CreateTextureFromSurface(renderer, connected_text_surf);
    SDL_FreeSurface(connected_text_surf);
    connected_text_surf = NULL;

/*  Start off networking thread */
    std::thread networkThread(Network::mainLoop, host, priority, priorityFactor, jpegQuality, QoS);
    printf("did network\n");

    printf("gonna start rendering\n");

    int lastJPEG = 0;

    SDL_Event event;
#ifdef __WIIU__
    while (WHBProcIsRunning()) {
#else
    while (1) {
#endif
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            break;
        }
        Network::State networkState = Network::GetNetworkState();

        if (networkState == Network::CONNECTED_STREAMING) {
            SDL_SetRenderDrawColor(renderer, bg_r, bg_g, bg_b, 0xFF);
        } else {
            SDL_SetRenderDrawColor(renderer, 0x7F, 0x7F, 0x7F, 0xFF);
        }
        SDL_RenderClear(renderer);

        if (networkState == Network::CONNECTED_STREAMING) {
            int cJPEG = Network::GetTopJPEGID();
            if (lastJPEG != cJPEG) {
                auto jpeg = Network::GetTopJPEG();
                int pitch;
                void* pixels;
                ret = SDL_LockTexture(topTexture, NULL, &pixels, &pitch);
                if (ret == 0) {
                    ret = tjDecompress2(tj_handle,
                        jpeg.data(), jpeg.size(), (uint8_t*)pixels,
                        240, pitch, 0,
                        TJPF_RGBA, 0
                    );

                    SDL_UnlockTexture(topTexture);

                    if (ret) {
                        printf("[Decoder] %s\n", tjGetErrorStr());
                    }
                    lastJPEG = cJPEG;
                } else {
                    printf("[Decoder] %s\n", SDL_GetError());
                }
            }

            SDL_Rect dstrect = {
                .x = (1280 - 720) / 2,
                .y = -(1200 - 720) / 2,
                .w = 720,
                .h = 1200,
            };
            SDL_RenderCopyEx(renderer, topTexture, NULL, &dstrect, 270, NULL, SDL_FLIP_NONE);
        } else if (networkState == Network::CONNECTING) {
            SDL_Rect dstrect = { 0 };
            SDL_QueryTexture(connecting_text, &format, &access, &dstrect.w, &dstrect.h);
            dstrect.y = 720 - dstrect.h;

            SDL_RenderCopy(renderer, connecting_text, NULL, &dstrect);
            dstrect.x += dstrect.w;

            int connect_attempts = Network::GetConnectionAttempts();
            if (connect_attempts > 0) {
                SDL_QueryTexture(attempt_text, &format, &access, &dstrect.w, &dstrect.h);
                dstrect.y = 720 - dstrect.h;

                SDL_RenderCopy(renderer, attempt_text, NULL, &dstrect);
                dstrect.x += dstrect.w;

                std::string attempts_num_str = std::to_string(connect_attempts);
                for (auto c : attempts_num_str) {
                    SDL_Texture* num_tex = numbers_text[c];

                    SDL_QueryTexture(num_tex, &format, &access, &dstrect.w, &dstrect.h);
                    dstrect.y = 720 - dstrect.h;

                    SDL_RenderCopy(renderer, num_tex, NULL, &dstrect);
                    dstrect.x += dstrect.w;
                }

            }
        } else if (networkState == Network::CONNECTED_WAIT) {
            SDL_Rect dstrect = { 0 };
            SDL_QueryTexture(connected_text, &format, &access, &dstrect.w, &dstrect.h);
            dstrect.y = 720 - dstrect.h;

            SDL_RenderCopy(renderer, connected_text, NULL, &dstrect);
            dstrect.x += dstrect.w;
        }

        SDL_RenderPresent(renderer);
    }

    printf("waiting for network to quit\n");

    Network::Quit();
    networkThread.join();

    printf("network quit!\n");

    //ProcUI hack lol
    //SDL_DestroyRenderer(renderer);
    //SDL_DestroyWindow(window);

    printf("done!\n");

    SDL_Quit();

    #ifdef __WIIU__
    printf("bye!\n");
    WHBLogUdpDeinit();
    WHBLogCafeDeinit();
    #endif

    #ifdef USE_RAMFS
    ramfsExit();
    #endif

    return 0;
}
