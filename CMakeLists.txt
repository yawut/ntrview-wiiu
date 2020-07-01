cmake_minimum_required(VERSION 3.16)

project(ntrview C CXX)
if (WIIU)
    include("${DEVKITPRO}/wut/share/wut.cmake" REQUIRED)
    set(PKG_CONFIG_EXECUTABLE "/opt/devkitpro/portlibs/wiiu/bin/powerpc-eabi-pkg-config")
endif()

find_package(SDL2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(libturbojpeg REQUIRED IMPORTED_TARGET libturbojpeg)

add_executable(ntrview
    main.cpp
    Network.cpp
)

target_compile_options(ntrview PRIVATE
    -Wall -Wextra
)

if (WIIU)
    target_compile_definitions(ntrview PRIVATE
        __WIIU__
    )
endif()

if (NOT WIIU)
    find_package(Threads)
    target_link_libraries(ntrview Threads::Threads)
endif()

if (TARGET SDL2::SDL2)
    target_link_libraries(ntrview SDL2::SDL2)
elseif (TARGET SDL2::SDL2-static)
    target_link_libraries(ntrview SDL2::SDL2-static)
else()
    error("SDL2 not found!")
endif()

target_link_libraries(ntrview PkgConfig::libturbojpeg)

if (WIIU)
    wut_create_rpx(ntrview)
endif()
