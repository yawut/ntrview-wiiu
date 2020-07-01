#include <SDL.h>
#include <turbojpeg.h>

#ifdef __WIIU__
#include <whb/log.h>
#include <whb/log_udp.h>
#include <whb/log_cafe.h>
#include <whb/proc.h>
#include <sys/iosupport.h>

static ssize_t wiiu_log_write(struct _reent* r, void* fd, const char* ptr, size_t len) {
    WHBLogPrintf("%*.*s", len, len, ptr);
    return len;
}
static devoptab_t dotab_stdout = {
    .name = "udp_out",
    .write_r = &wiiu_log_write,
};
#else
#include <stdio.h>
#endif

#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <turbojpeg.h>
#include <unistd.h>
#include <sys/types.h>
#include <thread>

#include "Network.hpp"

int main(int argc, char** argv) {
    int ret;
    (void)argc, (void)argv;

    #ifdef __WIIU__
    WHBLogUdpInit();
    WHBLogCafeInit();
    devoptab_list[STD_OUT] = &dotab_stdout;
    devoptab_list[STD_ERR] = &dotab_stdout;
    #warning "Building for Wii U"
    #endif

    printf("hi %ld\n", sizeof(Packet));

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Couldn't init SDL!\n");
        SDL_Quit();
        return 3;
    }

    printf("did init\n");

    Network& network = GetNetwork();
    std::thread networkThread(NetworkThread);
    printf("did network\n");

    uint32_t flags = 0;

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* topTexture;

    if (SDL_CreateWindowAndRenderer(1280, 720, flags, &window, &renderer)) {
        printf("Couldn't make window and renderer: %s\n", SDL_GetError());
        SDL_Quit();
        return 3;
    }

    topTexture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_RGBX8888, SDL_TEXTUREACCESS_STREAMING,
        240, 400
    );
    if (!topTexture) {
        printf("Couldn't make texture: %s\n", SDL_GetError());
        SDL_Quit();
        return 3;
    }

    tjhandle tj_handle = tjInitDecompress();

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

        int cJPEG = network.GetTopJPEGID();
        if (lastJPEG != cJPEG) {
            auto jpeg = network.GetTopJPEG();
            int pitch = TJPAD(240 * 4);
            void* pixels = malloc(pitch * 400);

            ret = tjDecompress2(tj_handle,
                jpeg.data(), jpeg.size(), (uint8_t*)pixels,
                240, pitch, 0,
                TJPF_RGBA, 0
            );
            if (ret == 0) {
                SDL_UpdateTexture(topTexture, NULL, pixels, pitch);
            } else {
                printf("jpeg err: %s\n", tjGetErrorStr());
            }
            lastJPEG = cJPEG;
            free(pixels);
        }

        SDL_SetRenderDrawColor(renderer, 0x7F, 0x7F, 0x7F, 0xFF);
        SDL_RenderClear(renderer);

        SDL_Rect dstrect = {
            .x = (1280 - 720) / 2,
            .y = -(1280 - 720) / 2,
            .w = 720,
            .h = 1280,
        };
        SDL_RenderCopyEx(renderer, topTexture, NULL, &dstrect, 270, NULL, SDL_FLIP_NONE);

        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    printf("done!\n");

    SDL_Quit();

    #ifdef __WIIU__
    WHBLogUdpDeinit();
    WHBLogCafeDeinit();
    #endif

    return 0;
}
