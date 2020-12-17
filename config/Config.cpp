#include "Config.hpp"

#include <inipp.h>
typedef inipp::Ini<char> Ini;

void configGetRect(Ini& ini, const std::string& section, const std::string& name, Gfx::Rect& rect) {
    inipp::extract(ini.sections[section][name + "x"], rect.x);
    inipp::extract(ini.sections[section][name + "y"], rect.y);
    inipp::extract(ini.sections[section][name + "w"], rect.d.w);
    inipp::extract(ini.sections[section][name + "h"], rect.d.h);
    int angle = -1;
    inipp::extract(ini.sections[section][name + "angle"], angle);
    switch (angle) {
        case 0: {
            rect.rotation = Gfx::GFX_ROTATION_0;
            break;
        }
        case 90: {
            rect.rotation = Gfx::GFX_ROTATION_90;
            break;
        }
        case 180: {
            rect.rotation = Gfx::GFX_ROTATION_180;
            break;
        }
        case 270: {
            rect.rotation = Gfx::GFX_ROTATION_270;
            break;
        }
        default: {
            break;
        }
    }
}

void Config::LoadINI(std::basic_istream<char>& is) {
    Ini ini;
    ini.parse(is);

    for (const auto& err : ini.errors) {
        printf("error in ntrview.ini: %s\n", err.c_str());
    }

    bool ret = false;

    inipp::extract(
        ini.sections["3ds"]["ip"],
        this->networkconfig.host
    );
    inipp::extract(
        ini.sections["3ds"]["priority"],
        this->networkconfig.priority
    );
    inipp::extract(
        ini.sections["3ds"]["priorityFactor"],
        this->networkconfig.priorityFactor
    );
    inipp::extract(
        ini.sections["3ds"]["jpegQuality"],
        this->networkconfig.jpegQuality
    );
    inipp::extract(
        ini.sections["3ds"]["QoS"],
        this->networkconfig.QoS
    );

    ret = inipp::extract(
        ini.sections["network"]["input_ratelimit"],
        this->networkconfig.input_ratelimit_us
    );
    if (ret) this->networkconfig.input_ratelimit_us *= 1000;
    ret = inipp::extract(
        ini.sections["network"]["input_pollrate"],
        this->networkconfig.input_pollrate_us
    );
    if (ret) this->networkconfig.input_pollrate_us *= 1000;

    inipp::extract(
        ini.sections["display"]["background_r"],
        this->background.r
    );
    inipp::extract(
        ini.sections["display"]["background_g"],
        this->background.r
    );
    inipp::extract(
        ini.sections["display"]["background_b"],
        this->background.r
    );

    for (int i = 0; i < 1; i++) {
        std::string profile_name = "profile:" + std::to_string(i);

        configGetRect(ini, profile_name, "layout_480p_tv_top_", this->profiles[i].layout_tv[Gfx::RESOLUTION_480P][0]);
        configGetRect(ini, profile_name, "layout_480p_tv_btm_", this->profiles[i].layout_tv[Gfx::RESOLUTION_480P][1]);

        configGetRect(ini, profile_name, "layout_720p_tv_top_", this->profiles[i].layout_tv[Gfx::RESOLUTION_720P][0]);
        configGetRect(ini, profile_name, "layout_720p_tv_btm_", this->profiles[i].layout_tv[Gfx::RESOLUTION_720P][1]);

        configGetRect(ini, profile_name, "layout_1080p_tv_top_", this->profiles[i].layout_tv[Gfx::RESOLUTION_1080P][0]);
        configGetRect(ini, profile_name, "layout_1080p_tv_btm_", this->profiles[i].layout_tv[Gfx::RESOLUTION_1080P][1]);

        configGetRect(ini, profile_name, "layout_drc_top_", this->profiles[i].layout_drc[0]);
        configGetRect(ini, profile_name, "layout_drc_btm_", this->profiles[i].layout_drc[1]);
    }
}
