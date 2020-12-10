#include <turbojpeg.h>
#include <inipp.h>

#include "gfx/Gfx.hpp"
#include "gfx/font/Text.hpp"
#include "input/Input.hpp"
#include "common.h"
#include "util.hpp"
#include "config/Config.hpp"

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
#include <fstream>

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

    Config config;

    printf("did init\n");

/*  Show loading text */
    Text::Text loading_text("Now Loading");
    Gfx::PrepRender();
    Gfx::PrepRenderBtm();
    Gfx::Clear(config.background);
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
    std::ifstream config_file(NTRVIEW_DIR "/ntrview.ini");
#else
    std::ifstream config_file("ntrview.ini");
#endif

    config.LoadINI(config_file);

    std::string connecting_text_str("Connecting to ");
    connecting_text_str.append(config.networkconfig.host);
    Text::Text connecting_text(connecting_text_str);

    Text::Text attempt_text(", attempt ");
    Text::Text connected_text("Connected.");
    Text::Text bad_ip_text("Bad IP - check your config");

/*  Start off networking thread */
    std::thread networkThread(Network::mainLoop, config.networkconfig);
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

        const auto& profile = config.profiles[config.profile];

        Gfx::PrepRender();
        Gfx::PrepRenderTop();

        if (networkState == Network::CONNECTED_STREAMING) {
            Gfx::Clear(config.background);

            if (profile.layout_tv[curRes][0].d.w) {
                topTexture.Render(profile.layout_tv[curRes][0]);
            }
            if (profile.layout_tv[curRes][1].d.w) {
                btmTexture.Render(profile.layout_tv[curRes][1]);
            }
        } else {
            Gfx::Clear(config.background);
        }

        Gfx::DoneRenderTop();
        Gfx::PrepRenderBtm();

        if (networkState == Network::CONNECTED_STREAMING) {
        #ifndef GFX_SDL
            Gfx::Clear(config.background);
        #endif

            if (profile.layout_drc[0].d.w) {
                topTexture.Render(profile.layout_drc[0]);
            }
            if (profile.layout_drc[1].d.w) {
                btmTexture.Render(profile.layout_drc[1]);
            }
        } else if (networkState == Network::CONNECTING) {
            Gfx::Clear(config.background);

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
            Gfx::Clear(config.background);
            connected_text.Render(connected_text.baseline_y, 480 - connected_text.d.h);
        } else if (networkState == Network::ERR_BAD_IP) {
            Gfx::Clear(config.background);
            bad_ip_text.Render(bad_ip_text.baseline_y, 480 - bad_ip_text.d.h);
        }

        Gfx::DoneRenderBtm();
        Gfx::Present();

        if (networkState == Network::CONNECTED_STREAMING) {
            auto input = Input::Get(profile.layout_drc[1]);
            if (input) {
                Network::Input(*input);
            }
        }
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
