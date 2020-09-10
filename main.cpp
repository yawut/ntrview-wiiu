#include <turbojpeg.h>
#include <INIReader.h>

#include "gfx/Gfx.hpp"
#include "common.h"
#include "util.hpp"

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
    bool bret;
    (void)argc, (void)argv;

    #ifdef USE_RAMFS
    ramfsInit();
    OnLeavingScope rfs_c([&] {
        ramfsExit();
    });
    #endif

    #ifdef __WIIU__
    WHBLogUdpInit();
    WHBLogCafeInit();
    devoptab_list[STD_OUT] = &dotab_stdout;
    devoptab_list[STD_ERR] = &dotab_stdout;
    #warning "Building for Wii U"
    #ifndef GFX_SDL
    WHBProcInit();
    #endif
    OnLeavingScope prc_c([&] {
        printf("bye!\n");
        WHBLogUdpDeinit();
        WHBLogCafeDeinit();
        #ifndef GFX_SDL
        WHBProcShutdown();
        #endif
    });
    #endif

    printf("hi\n");

    bret = Gfx::Init();
    OnLeavingScope gfx_c([&] {
        Gfx::Quit();
    });
    if (!bret) {
        printf("Graphics init error!\n");
        return 3;
    }
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
        return 3;
    }

    Gfx::Texture btmTexture(240, 320);
    if (!btmTexture.valid()) {
        printf("Couldn't make texture: %s\n", Gfx::GetError());
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

    int lastTopJPEG = 0;
    int lastBtmJPEG = 0;

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
            int cTopJPEG = Network::GetTopJPEGID();
            if (lastTopJPEG != cTopJPEG) {
                auto jpeg = Network::GetTopJPEG(cTopJPEG);

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
                    lastTopJPEG = cTopJPEG;
                } else {
                    printf("[Decoder] Error: %s\n", Gfx::GetError());
                }
            }
            int cBtmJPEG = Network::GetBtmJPEGID();
            if (lastBtmJPEG != cBtmJPEG) {
                auto jpeg = Network::GetBtmJPEG(cBtmJPEG);

                auto pixels = btmTexture.Lock();
                if (btmTexture.locked && !pixels.empty()) {
                    ret = tjDecompress2(tj_handle,
                        jpeg.data(), jpeg.size(), pixels.data(),
                        btmTexture.d.w, btmTexture.pitch, 0,
                        TJPF_RGBA, 0
                    );

                    btmTexture.Unlock(pixels);

                    if (ret) {
                        printf("[Decoder] %s\n", tjGetErrorStr());
                    }
                    lastBtmJPEG = cBtmJPEG;
                } else {
                    printf("[Decoder] Error: %s\n", Gfx::GetError());
                }
            }
        }

        Gfx::PrepRender();
        Gfx::PrepRenderTop();

        if (networkState == Network::CONNECTED_STREAMING) {
            Gfx::Clear((Gfx::rgb) { .r = bg_r, .g = bg_g, .b = bg_b });
            Gfx::Rect dstrect = {
                .x = 40,
                .y = 0,
                .d = {
                    .w = 1200,
                    .h = 720,
                },
                .rotation = Gfx::GFX_ROTATION_270,
            };
            topTexture.Render(dstrect);
        } else {
            Gfx::Clear((Gfx::rgb) { .r = 0x7f, .g = 0x7f, .b = 0x7f });
        }

        Gfx::DoneRenderTop();
        Gfx::PrepRenderBtm();

        if (networkState == Network::CONNECTED_STREAMING) {
        #ifndef GFX_SDL
            Gfx::Clear((Gfx::rgb) { .r = bg_r, .g = bg_g, .b = bg_b });
        #endif
            Gfx::Rect dstrect = {
                .x = 107,
                .y = 0,
                .d = {
                    .w = 640,
                    .h = 480,
                },
                .rotation = Gfx::GFX_ROTATION_270,
            };
            btmTexture.Render(dstrect);
        } else if (networkState == Network::CONNECTING) {
            Gfx::Clear((Gfx::rgb) { .r = 0x7f, .g = 0x7f, .b = 0x7f });
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
            Gfx::Clear((Gfx::rgb) { .r = 0x7f, .g = 0x7f, .b = 0x7f });
            connected_text.Render((Gfx::Rect) {
                .x = 0,
                .y = 720 - connected_text.d.h,
                .d = connected_text.d,
            });
        }

        Gfx::DoneRenderBtm();
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

    return 0;
}
