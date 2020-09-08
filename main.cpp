#include <turbojpeg.h>
#include <INIReader.h>

#include "gfx/Gfx.hpp"
#include "common.h"

#ifdef __WIIU__
#include <whb/log.h>
#include <whb/log_udp.h>
#include <whb/log_cafe.h>
#include <whb/proc.h>
#include <sys/iosupport.h>
#include <romfs-wiiu.h>

static ssize_t wiiu_log_write(struct _reent* r, void* fd, const char* ptr, size_t len) {
    (void)r; (void)fd;
    WHBLogPrintf("%*.*s", len, len, ptr);
    return len;
}
static devoptab_t dotab_stdout = {
    .name = "udp_out",
    .write_r = &wiiu_log_write,
};
#endif

#include <stdbool.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <thread>

#include "Network.hpp"

int main(int argc, char** argv) {
    int ret;
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

    Gfx::Init();
    tjhandle tj_handle = tjInitDecompress();

    printf("did init\n");

/*  Show loading text */
    Gfx::Texture loading_text("Now Loading");
    Gfx::Clear((Gfx::rgb) { .r = 0x7f, .g = 0x7f, .b = 0x7f });
    loading_text.Render((Gfx::Rect) {
        .x = 0,
        .y = 720 - loading_text.d.h,
        .d = loading_text.d,
    });
    Gfx::Present();

/*  Allocate textures for the received frames */
    Gfx::Texture topTexture(240, 400);
    if (!topTexture.valid()) {
        printf("Couldn't make texture: %s\n", Gfx::GetError());
        Gfx::Quit();
        return 3;
    }

    Gfx::Texture btmTexture(240, 320);
    if (!btmTexture.valid()) {
        printf("Couldn't make texture: %s\n", Gfx::GetError());
        Gfx::Quit();
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
    char numbers[] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', '.' };
    for (char num : numbers) {
        Gfx::CacheNumber(num);
    }

    std::string connecting_text_str("Connecting to ");
    connecting_text_str.append(host);
    Gfx::Texture connecting_text(connecting_text_str);

    Gfx::Texture attempt_text(", attempt ");
    Gfx::Texture connected_text("Connected.");

/*  Start off networking thread */
    std::thread networkThread(Network::mainLoop, host, priority, priorityFactor, jpegQuality, QoS);
    printf("did network\n");

    printf("gonna start rendering\n");

    int lastJPEG = 0;

#ifdef GFX_SDL
    SDL_Event event;
#endif
#ifdef __WIIU__
    while (WHBProcIsRunning()) {
#else
    while (1) {
#endif
#ifdef GFX_SDL
        SDL_PollEvent(&event);
        if (event.type == SDL_QUIT) {
            break;
        }
#endif
        Network::State networkState = Network::GetNetworkState();

        if (networkState == Network::CONNECTED_STREAMING) {
            Gfx::Clear((Gfx::rgb) { .r = bg_r, .g = bg_g, .b = bg_b });
        } else {
            Gfx::Clear((Gfx::rgb) { .r = 0x7f, .g = 0x7f, .b = 0x7f });
        }

        if (networkState == Network::CONNECTED_STREAMING) {
            int cJPEG = Network::GetTopJPEGID();
            if (lastJPEG != cJPEG) {
                auto jpeg = Network::GetTopJPEG(cJPEG);

                auto pixels = topTexture.Lock();
                if (topTexture.locked && !pixels.empty()) {
                    ret = tjDecompress2(tj_handle,
                        jpeg.data(), jpeg.size(), pixels.data(),
                        topTexture.d.w, topTexture.pitch, 0,
                        TJPF_RGBA, 0
                    );

                    topTexture.Unlock(pixels);

                    if (ret) {
                        printf("[Decoder] %s\n", tjGetErrorStr());
                    }
                    lastJPEG = cJPEG;
                } else {
                    printf("[Decoder] Error: %s\n", Gfx::GetError());
                }
            }

            Gfx::Rect dstrect = {
                .x = (1280 - 720) / 2,
                .y = -(1200 - 720) / 2,
                .d = {
                    .w = 720,
                    .h = 1200,
                },
                .angle = 270,
            };
            topTexture.Render(dstrect);
        } else if (networkState == Network::CONNECTING) {
            int x = 0;
            connecting_text.Render((Gfx::Rect) {
                .x = x,
                .y = 720 - connecting_text.d.h,
                .d = connecting_text.d,
            });
            x += connecting_text.d.w;

            int connect_attempts = Network::GetConnectionAttempts();
            if (connect_attempts > 0) {
                attempt_text.Render((Gfx::Rect) {
                    .x = x,
                    .y = 720 - attempt_text.d.h,
                    .d = attempt_text.d,
                });
                x += attempt_text.d.w;

                std::string attempts_num_str = std::to_string(connect_attempts);
                for (auto c : attempts_num_str) {
                    auto num_tex_o = Gfx::GetCachedNumber(c);
                    if (!num_tex_o) continue;
                    auto& num_tex = num_tex_o->get();

                    num_tex.Render((Gfx::Rect) {
                        .x = x,
                        .y = 720 - num_tex.d.h,
                        .d = num_tex.d,
                    });
                    x += num_tex.d.w;
                }
            }
        } else if (networkState == Network::CONNECTED_WAIT) {
            connected_text.Render((Gfx::Rect) {
                .x = 0,
                .y = 720 - connected_text.d.h,
                .d = connected_text.d,
            });
        }

        Gfx::Present();
    }

    printf("waiting for network to quit\n");

    Network::Quit();
    networkThread.join();

    printf("network quit!\n");

    //ProcUI hack lol
    //SDL_DestroyRenderer(renderer);
    //SDL_DestroyWindow(window);

    printf("done!\n");

    Gfx::Quit();

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
