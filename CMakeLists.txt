cmake_minimum_required(VERSION 3.16)

project(ntrview C CXX)
if (WIIU)
    include("${WUT_ROOT}/share/wut.cmake" REQUIRED)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(libturbojpeg REQUIRED IMPORTED_TARGET libturbojpeg)

add_subdirectory(ext/inih)

add_executable(ntrview
    main.cpp
    Network.cpp
)
target_include_directories(ntrview PRIVATE
    .
)
set_property(TARGET ntrview PROPERTY CXX_STANDARD 20)
target_compile_options(ntrview PRIVATE
    -Wall -Wextra -Wno-missing-field-initializers
)

if (WIIU)
    wut_create_rpx(ntrview)
    target_compile_definitions(ntrview PRIVATE
        __WIIU__
    )

    set(NTRVIEW_GFX_SDL OFF)
    set(NTRVIEW_GFX_GX2 ON)
    set(NTRVIEW_USE_RAMFS ON)
elseif (NOT WIIU)
    find_package(Threads)
    target_link_libraries(ntrview Threads::Threads)

    set(NTRVIEW_GFX_SDL ON)
    set(NTRVIEW_GFX_GX2 OFF)
    set(NTRVIEW_USE_RAMFS OFF)
endif()

if (NTRVIEW_GFX_SDL)
    target_sources(ntrview PRIVATE
        gfx/Gfx_SDL.cpp
    )
    target_compile_definitions(ntrview PRIVATE
        GFX_SDL
    )

    find_package(SDL2 REQUIRED)
    pkg_check_modules(SDL2_ttf REQUIRED IMPORTED_TARGET SDL2_ttf)

    if (TARGET SDL2::SDL2)
        target_link_libraries(ntrview SDL2::SDL2 PkgConfig::SDL2_ttf)
    elseif (TARGET SDL2::SDL2-static)
        target_link_libraries(ntrview SDL2::SDL2-static PkgConfig::SDL2_ttf)
    else()
        error("SDL2 not found!")
    endif()
elseif (NTRVIEW_GFX_GX2)
    target_sources(ntrview PRIVATE
        gfx/Gfx_GX2.cpp
    )
    target_compile_definitions(ntrview PRIVATE
        GFX_GX2
    )

    add_subdirectory(gfx/gx2_shaders)
    target_link_libraries(ntrview gx2shaders)
endif()

target_link_libraries(ntrview
    PkgConfig::libturbojpeg
    inih::inih-cpp
)

if (NTRVIEW_USE_RAMFS)
    target_compile_definitions(ntrview PRIVATE
        USE_RAMFS
    )
    add_custom_command(
        OUTPUT app.romfs.o
        COMMAND tar -H ustar -cvf romfs.tar -C "${CMAKE_CURRENT_SOURCE_DIR}/resin" .
        COMMAND ${CMAKE_LINKER} --relocatable --format binary --output app.romfs.o romfs.tar
        COMMAND rm -f romfs.tar
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resin"
    )
    set_source_files_properties(
        app.romfs.o
        PROPERTIES
        EXTERNAL_OBJECT true
        GENERATED true
    )
    add_library(romfs
        app.romfs.o
        ext/resinfs/source/romfs.c
    )
    target_include_directories(romfs
        PUBLIC
            ext/resinfs/include
    )

    target_link_libraries(ntrview romfs)
endif() #NTRVIEW_USE_ROMFS
