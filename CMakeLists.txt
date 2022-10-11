cmake_minimum_required(VERSION 3.21)
project(dither VERSION 0.0.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(deps/stb)
find_package(glm REQUIRED)
find_package(asio REQUIRED)

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

