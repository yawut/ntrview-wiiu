#include "JPEG.h"

#include <cstdio>
#include <sys/stat.h>

Gfx::Texture Gfx::LoadFromJPEG(tjhandle tj_handle, const std::filesystem::path& path) {
    FILE* fd = fopen(path.c_str(), "rb");
    if (!fd)
        return {};

    struct stat stats;
    if(fstat(fileno(fd), &stats))
        return {};
    auto size = stats.st_size;

    std::vector<unsigned char> buffer(size);
    auto res = fread(buffer.data(), size, 1, fd);
    fclose(fd);
    if (res != 1) return {};

    int width, height, subsamp, colourspace;
    int ret = tjDecompressHeader3(tj_handle,
                                  buffer.data(), buffer.size(),
                                  &width, &height, &subsamp, &colourspace);
    if (ret) {
        printf("[Textures] %s\n", tjGetErrorStr());
        return {};
    }

    Gfx::Texture tex(width, height);
    auto pixels = tex.Lock();
    if (tex.locked && !pixels.empty()) {
        ret = tjDecompress2(tj_handle,
                            buffer.data(), buffer.size(), pixels.data(),
                            width, 0, height,
                            TJPF_RGBA, 0);

        tex.Unlock(pixels);

        if (ret) {
            printf("[Textures] %s\n", tjGetErrorStr());
            return {};
        }
    } else {
        printf("[Textures] Error: %s\n", Gfx::GetError());
        return {};
    }

    return tex;
}
