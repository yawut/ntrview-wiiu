set(CMAKE_TOOLCHAIN_FILE "$ENV{DEVKITPRO}/wut/share/wut.toolchain.cmake")
cmake_minimum_required(VERSION 3.16)

project(ntrview C CXX)

add_executable(ntrview
    main.cpp
    Network.cpp
    config/Config.cpp
    menu/StatusOverlay.cpp
    input/Input_wiiu.cpp
    gfx/Gfx_GX2.cpp
    gfx/Targa.cpp
    gfx/JPEG.cpp
    gfx/font/Text_FreeType2.cpp
    menu/Menu.cpp
)
wut_create_rpx(ntrview)
wut_create_wuhb(ntrview
    CONTENT content
    NAME NTRView Remote Viewer
    SHORTNAME NTRView
    AUTHOR quarky
    ICON dist/ntrview-iconTex.tga
    TVSPLASH dist/ntrview-bootTvTex.tga
    DRCSPLASH dist/ntrview-bootDrcTex.tga
)

target_include_directories(ntrview PRIVATE
    .
    ext/inipp/inipp
)
set_property(TARGET ntrview PROPERTY CXX_STANDARD 20)
target_compile_options(ntrview PRIVATE
    -Wall -Wextra -Wno-missing-field-initializers
)

find_package(PkgConfig REQUIRED)
pkg_check_modules(libturbojpeg REQUIRED IMPORTED_TARGET libturbojpeg)
pkg_check_modules(freetype2 REQUIRED IMPORTED_TARGET freetype2)
add_subdirectory(gfx/gx2_shaders)
target_link_libraries(ntrview
    gx2shaders PkgConfig::freetype2 PkgConfig::libturbojpeg
)

# This build used to have SDL2 support, so these feature defines are left over
target_compile_definitions(ntrview PRIVATE
    __WIIU__ GFX_GX2 TXT_FT2 INPUT_ENABLE INPUT_WIIU
)
