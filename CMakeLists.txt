cmake_minimum_required(VERSION 3.21)
project(imagepp VERSION 0.0.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)         # Generate compile_commands.json for ccls
set_property(GLOBAL PROPERTY USE_FOLDERS ON)  # Group CMake targets inside a folder

add_subdirectory(deps/stb)
find_package(glm CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(Threads REQUIRED)

project(image)
set(TARGET_SOURCE_FILES
    "image.hpp"
    "image.cpp"
)
add_library(${PROJECT_NAME} OBJECT ${TARGET_SOURCE_FILES})
target_include_directories(${PROJECT_NAME} PRIVATE
    "${PROJECT_SOURCE_DIR}"
)
target_link_libraries(${PROJECT_NAME}
    stb
    glm::glm
    Threads::Threads
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
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        "/W4"
        "/WX"
    )
endif()
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

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
    glm::glm
    asio::asio
    Threads::Threads
    image
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
    )
else()
    target_compile_options(${PROJECT_NAME} PRIVATE
        "/W4"
        "/WX"
    )
endif()
source_group(TREE "${CMAKE_CURRENT_LIST_DIR}" FILES ${TARGET_SOURCE_FILES})

