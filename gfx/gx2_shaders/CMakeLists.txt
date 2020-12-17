project(gx2shaders LANGUAGES C)

function(shader target name)
    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/shader_${name}.c
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build-shaders.sh
            ${CMAKE_CURRENT_SOURCE_DIR}/${name}
            ${CMAKE_CURRENT_BINARY_DIR}/shader_${name}.c
            ${name}
        DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/build-shaders.sh
            ${CMAKE_CURRENT_SOURCE_DIR}/${name}.vsh
            ${CMAKE_CURRENT_SOURCE_DIR}/${name}.psh
        COMMENT "Assembling shaders"
    )
    target_sources(${target} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/shader_${name}.c)
endfunction()

add_library(gx2shaders)
shader(gx2shaders main)
shader(gx2shaders text)
shader(gx2shaders colour)

target_include_directories(gx2shaders
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)
