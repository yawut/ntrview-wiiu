project(gx2shaders LANGUAGES C)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shaders.c
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build-shaders.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/main
        ${CMAKE_CURRENT_BINARY_DIR}/shaders.c
        main
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/build-shaders.sh
        ${CMAKE_CURRENT_SOURCE_DIR}/main.vsh
        ${CMAKE_CURRENT_SOURCE_DIR}/main.psh
    COMMENT "Assembling shaders"
)
add_library(gx2shaders ${CMAKE_CURRENT_BINARY_DIR}/shaders.c)

target_include_directories(gx2shaders
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
