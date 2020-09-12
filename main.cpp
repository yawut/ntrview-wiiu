#include <turbojpeg.h>
#include <INIReader.h>

#include "gfx/Gfx.hpp"
#include "gfx/font/Text.hpp"
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

const static Gfx::rgb builtin_bg = {
    .r = 0x20,
    .g = 0x00,
    .b = 0x27,
};
static Gfx::rgb user_bg = builtin_bg;

Gfx::Rect configGetRect(INIReader& config, const std::string& section, const std::string& name, Gfx::Rect defaults) {
    Gfx::Rect out;
    out.x = config.GetInteger(section, name + "x", defaults.x);
    out.y = config.GetInteger(section, name + "y", defaults.y);
    out.d.w = config.GetInteger(section, name + "w", defaults.d.w);
    out.d.h = config.GetInteger(section, name + "h", defaults.d.h);
    int angle = config.GetInteger(section, name + "angle", -1);
    switch (angle) {
        case 0: {
            out.rotation = Gfx::GFX_ROTATION_0;
            break;
        }
        case 90: {
            out.rotation = Gfx::GFX_ROTATION_90;
            break;
        }
        case 180: {
            out.rotation = Gfx::GFX_ROTATION_180;
            break;
        }
        case 270: {
            out.rotation = Gfx::GFX_ROTATION_270;
            break;
        }
        default: {
            out.rotation = defaults.rotation;
            break;
        }
    }
    return out;
}

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
    Gfx::Resolution curRes = Gfx::GetResolution();

    bret = Text::Init();
    OnLeavingScope txt_c([&] {
        Text::Quit();
    });
    if (!bret) {
        printf("Text init error!\n");
        return 3;
    }

    tjhandle tj_handle = tjInitDecompress();

    printf("did init\n");

/*  Show loading text */
    Text::Text loading_text("Now Loading");
    Gfx::PrepRender();
    Gfx::PrepRenderBtm();
    Gfx::Clear(user_bg);
    loading_text.Render(loading_text.baseline_y, 480 - loading_text.d.h);
    Gfx::DoneRenderBtm();
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
    user_bg.r = config.GetInteger("display", "background_r", builtin_bg.r);
    user_bg.r = config.GetInteger("display", "background_g", builtin_bg.g);
    user_bg.r = config.GetInteger("display", "background_b", builtin_bg.b);

    Gfx::Rect layout_tv[Gfx::RESOLUTION_MAX][2 /*inputs*/];
    layout_tv[Gfx::RESOLUTION_480P][0] = configGetRect(config, "profile:0", "layout_480p_tv_top_", (Gfx::Rect) {
        .x = 27,
        .y = 0,
        .d = {
            .w = 800,
            .h = 480,
        },
        .rotation = Gfx::GFX_ROTATION_270,
    });
    layout_tv[Gfx::RESOLUTION_480P][1] = configGetRect(config, "profile:0", "layout_480p_tv_btm_", (Gfx::Rect) {
        .d = {
            .w = 0,
        },
        .rotation = Gfx::GFX_ROTATION_270,
    });
    layout_tv[Gfx::RESOLUTION_720P][0] = configGetRect(config, "profile:0", "layout_720p_tv_top_", (Gfx::Rect) {
        .x = 40,
        .y = 0,
        .d = {
            .w = 1200,
            .h = 720,
        },
        .rotation = Gfx::GFX_ROTATION_270,
    });
    layout_tv[Gfx::RESOLUTION_720P][1] = configGetRect(config, "profile:0", "layout_720p_tv_btm_", (Gfx::Rect) {
        .d = {
            .w = 0,
        },
        .rotation = Gfx::GFX_ROTATION_270,
    });
    layout_tv[Gfx::RESOLUTION_1080P][0] = configGetRect(config, "profile:0", "layout_1080p_tv_top_", (Gfx::Rect) {
        .x = 60,
        .y = 0,
        .d = {
            .w = 1800,
            .h = 1080,
        },
        .rotation = Gfx::GFX_ROTATION_270,
    });
    layout_tv[Gfx::RESOLUTION_1080P][1] = configGetRect(config, "profile:0", "layout_1080p_tv_btm_", (Gfx::Rect) {
        .d = {
            .w = 0,
        },
        .rotation = Gfx::GFX_ROTATION_270,
    });
    Gfx::Rect layout_drc[2];
    layout_drc[0] = configGetRect(config, "profile:0", "layout_drc_top_", (Gfx::Rect) {
        .d = {
            .w = 0,
        },
        .rotation = Gfx::GFX_ROTATION_270,
    });
    layout_drc[1] = configGetRect(config, "profile:0", "layout_drc_btm_", (Gfx::Rect) {
        .x = 107,
        .y = 0,
        .d = {
            .w = 640,
            .h = 480,
        },
        .rotation = Gfx::GFX_ROTATION_270,
    });

    std::string connecting_text_str("Connecting to ");
    connecting_text_str.append(host);
    Text::Text connecting_text(connecting_text_str);

    Text::Text attempt_text(", attempt ");
    Text::Text connected_text("Connected.");

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
            Gfx::Clear(user_bg);

            if (layout_tv[curRes][0].d.w) {
                topTexture.Render(layout_tv[curRes][0]);
            }
            if (layout_tv[curRes][1].d.w) {
                btmTexture.Render(layout_tv[curRes][1]);
            }
        } else {
            Gfx::Clear(builtin_bg);
        }

        Gfx::DoneRenderTop();
        Gfx::PrepRenderBtm();

        if (networkState == Network::CONNECTED_STREAMING) {
        #ifndef GFX_SDL
            Gfx::Clear(user_bg);
        #endif

            if (layout_drc[0].d.w) {
                topTexture.Render(layout_drc[0]);
            }
            if (layout_drc[1].d.w) {
                btmTexture.Render(layout_drc[1]);
            }
        } else if (networkState == Network::CONNECTING) {
            Gfx::Clear(builtin_bg);

            int x = connecting_text.baseline_y;
            connecting_text.Render(x, 480 - connecting_text.d.h);
            x += connecting_text.d.w;

            int connect_attempts = Network::GetConnectionAttempts();
            if (connect_attempts > 0) {
                attempt_text.Render(x, 480 - attempt_text.d.h);
                x += attempt_text.d.w;

                std::string attempts_num_str = std::to_string(connect_attempts);
                Text::Text attempts_num(attempts_num_str);
                attempts_num.Render(x, 480 - attempts_num.d.h);
                x += attempts_num.d.w;
            }
        } else if (networkState == Network::CONNECTED_WAIT) {
            Gfx::Clear(builtin_bg);
            connected_text.Render(connected_text.baseline_y, 480 - connected_text.d.h);
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
