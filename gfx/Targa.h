#pragma once

#include "Gfx.hpp"
#include <filesystem>
#include <turbojpeg.h>

namespace Gfx {
    Texture LoadFromTGA(const std::filesystem::path& path);
}
