cmake_minimum_required(VERSION 3.12)
project(stb)
add_library(${PROJECT_NAME} STATIC
    "${PROJECT_SOURCE_DIR}/stb_image_write.h"
    "${PROJECT_SOURCE_DIR}/stb_image.h"
    "${PROJECT_SOURCE_DIR}/stb_image.cpp"
)
target_include_directories(${PROJECT_NAME} PUBLIC
    "${PROJECT_SOURCE_DIR}"
)
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
