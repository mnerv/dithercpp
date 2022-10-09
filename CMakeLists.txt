cmake_minimum_required(VERSION 3.12)
project(dither VERSION 0.0.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(deps/stb)
add_library(glm INTERFACE)
target_include_directories(glm INTERFACE "${glm_DIR}")
if (NOT DEFINED glm_DIR)
    message(FATAL_ERROR "glm_DIR must be set and point to where glm library is. -Dglm_DIR=")
endif()

project(dither)
set(TARGET_SOURCE_FILES
    "dither.cpp"
)
add_executable(${PROJECT_NAME} ${TARGET_SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(${PROJECT_NAME}
    stb
    glm
)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
if (NOT MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        "-Wall"
        "-Wextra"
        "-Wconversion"
        "-Wpedantic"
        "-Wshadow"
        "-Werror"

        # glm warnings
        "-Wno-deprecated-volatile"
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        "/W4"
        "/WX"
    )
endif()

source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

