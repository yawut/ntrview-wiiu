cmake_minimum_required(VERSION 3.2)

project(inih
    LANGUAGES C CXX
    VERSION 51
)

add_library(inih
    ${CMAKE_CURRENT_SOURCE_DIR}/ini.c
)
add_library(inih::inih ALIAS inih)

target_include_directories(inih
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(inih-cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/cpp/INIReader.cpp
)
add_library(inih::inih-cpp ALIAS inih-cpp)

target_include_directories(inih-cpp
    PUBLIC
        ${PROJECT_SOURCE_DIR}/cpp
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(inih-cpp
    PUBLIC
        inih::inih
)
