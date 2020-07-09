cmake_minimum_required(VERSION 3.16)

project(ntrview C CXX)
if (WIIU)
    include("${WUT_ROOT}/share/wut.cmake" REQUIRED)
endif()

find_package(SDL2 REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(libturbojpeg REQUIRED IMPORTED_TARGET libturbojpeg)
pkg_check_modules(SDL2_ttf REQUIRED IMPORTED_TARGET SDL2_ttf)
add_subdirectory(ext/inih)

add_executable(ntrview
    main.cpp
    Network.cpp

    app.romfs.o
)

target_compile_options(ntrview PRIVATE
    -Wall -Wextra -Wno-missing-field-initializers
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

target_link_libraries(ntrview
    PkgConfig::libturbojpeg
    PkgConfig::SDL2_ttf
    inih::inih-cpp
)

if (WIIU)

#romfs
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

endif() #WIIU

if (WIIU)
    wut_create_rpx(ntrview)
endif()
