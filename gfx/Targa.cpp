#include "Targa.h"

#include <cstdio>
#include <sys/stat.h>

Gfx::Texture Gfx::LoadFromTGA(const std::filesystem::path& path) {
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

    // C moment
    int header_len = 0x12;
    int id_len = buffer[0];
    int img_type = buffer[2];
    int width = buffer[0xc] | (buffer[0xd] << 8);
    int height = buffer[0xe] | (buffer[0xf] << 8);
    int depth = buffer[0x10];
    int flag = buffer[0x11];

    int img_size = width * height * (depth / 8);
    if (header_len + id_len + img_size >= size || img_type != 2 || depth != 32 || flag != 8)
        return {};

    Gfx::Texture tex(width, height);
    auto pixels = tex.Lock();
    std::copy(
            buffer.begin() + header_len + id_len,
            buffer.begin() + header_len + id_len + img_size,
            pixels.begin()
    );
    tex.Unlock(pixels);

    return tex;
}
