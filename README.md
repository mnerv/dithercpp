# Post Prcessing in C++

Post processing images with C++. The goal is to implement cool algorithms that process images like the Floyd-Stenberg dithering.

## Floyd–Steinberg dithering

Dithering implementation in C++ with `stb_image` and `glm`.

## Requirements

  - [cmake](https://cmake.org)
  - [vcpkg](https://vcpkg.io/en/index.html)

## Development

Setup the development environment with `cmake`.

```
cmake -S . -Bbuild -GNinja -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
```

`${VCPKG_ROOT}`: Follow their installation instruction and set this variable so that it points to where vcpkg is installed.

