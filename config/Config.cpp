#include "Config.hpp"

#include <string>

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

Ini::Section configSerialiseRect(const std::string& name, const Gfx::Rect& rect) {
    int angle;
    switch (rect.rotation) {
        case Gfx::GFX_ROTATION_0: {
            angle = 0;
            break;
        }
        case Gfx::GFX_ROTATION_90: {
            angle = 90;
            break;
        }
        case Gfx::GFX_ROTATION_180: {
            angle = 180;
            break;
        }
        default:
        case Gfx::GFX_ROTATION_270: {
            angle = 270;
            break;
        }
    }
    return (Ini::Section) {
        {name + "x", std::to_string(rect.x)},
        {name + "y", std::to_string(rect.y)},
        {name + "w", std::to_string(rect.d.w)},
        {name + "h", std::to_string(rect.d.h)},
        {name + "angle", std::to_string(angle)},
    };
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
    //these are uint8_ts which messes with inipp::extract
    if (!ini.sections["3ds"]["priority"].empty()) {
        this->networkconfig.priority = std::stoi(ini.sections["3ds"]["priority"]);
    }
    if (!ini.sections["3ds"]["priorityFactor"].empty()) {
        this->networkconfig.priorityFactor = std::stoi(ini.sections["3ds"]["priorityFactor"]);
    }
    if (!ini.sections["3ds"]["jpegQuality"].empty()) {
        this->networkconfig.jpegQuality = std::stoi(ini.sections["3ds"]["jpegQuality"]);
    }
    if (!ini.sections["3ds"]["QoS"].empty()) {
        this->networkconfig.QoS = std::stoi(ini.sections["3ds"]["QoS"]);
    }

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

    if (!ini.sections["display"]["background_r"].empty()) {
        this->background.r = std::stoi(ini.sections["display"]["background_r"]);
    }
    if (!ini.sections["display"]["background_g"].empty()) {
        this->background.g = std::stoi(ini.sections["display"]["background_g"]);
    }
    if (!ini.sections["display"]["background_b"].empty()) {
        this->background.b = std::stoi(ini.sections["display"]["background_b"]);
    }

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

void Config::SaveINI(std::basic_ostream<char>& os) {
    Ini ini;

    ini.sections.emplace("3ds", (Ini::Section) {
        {"ip",             this->networkconfig.host},
        {"priority",       std::to_string(this->networkconfig.priority)},
        {"priorityFactor", std::to_string(this->networkconfig.priorityFactor)},
        {"jpegQuality",    std::to_string(this->networkconfig.jpegQuality)},
        {"QoS",            std::to_string(this->networkconfig.QoS)},
    });

    ini.sections.emplace("network", (Ini::Section) {
        {"input_ratelimit", std::to_string(this->networkconfig.input_ratelimit_us/1000)},
        {"input_pollrate",  std::to_string(this->networkconfig.input_pollrate_us/1000)},
    });

    ini.sections.emplace("display", (Ini::Section) {
        {"background_r", std::to_string(this->background.r)},
        {"background_g", std::to_string(this->background.g)},
        {"background_b", std::to_string(this->background.b)},
    });

    for (int i = 0; i < 1; i++) {
        std::string profile_name = "profile:" + std::to_string(i);

        Ini::Section profile;
        profile.merge(configSerialiseRect("layout_480p_tv_top_", this->profiles[i].layout_tv[Gfx::RESOLUTION_480P][0]));
        profile.merge(configSerialiseRect("layout_480p_tv_btm_", this->profiles[i].layout_tv[Gfx::RESOLUTION_480P][1]));

        profile.merge(configSerialiseRect("layout_720p_tv_top_", this->profiles[i].layout_tv[Gfx::RESOLUTION_720P][0]));
        profile.merge(configSerialiseRect("layout_720p_tv_btm_", this->profiles[i].layout_tv[Gfx::RESOLUTION_720P][1]));

        profile.merge(configSerialiseRect("layout_1080p_tv_top_", this->profiles[i].layout_tv[Gfx::RESOLUTION_1080P][0]));
        profile.merge(configSerialiseRect("layout_1080p_tv_btm_", this->profiles[i].layout_tv[Gfx::RESOLUTION_1080P][1]));

        profile.merge(configSerialiseRect("layout_drc_top_", this->profiles[i].layout_drc[0]));
        profile.merge(configSerialiseRect("layout_drc_btm_", this->profiles[i].layout_drc[1]));

        //slightly weird syntax to hit the move constructor
        ini.sections.emplace(profile_name, std::move(profile));
    }

    ini.generate(os);
}
