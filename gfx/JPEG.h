#pragma once

#include "Gfx.hpp"
#include <filesystem>
#include <turbojpeg.h>

namespace Gfx {
    Texture LoadFromJPEG(tjhandle tj_handle, const std::filesystem::path& path);
}
