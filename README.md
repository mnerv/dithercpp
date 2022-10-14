# Post Prcessing in C++

## Floyd–Steinberg dithering

Dithering implementation in C++ with `stb_image` and `glm`.

## Requires

  - [cmake](https://cmake.org)
  - [vcpkg](https://vcpkg.io/en/index.html)

## Development

Setup the development environment with `cmake`.

```
cmake -S . -Bbuild -GNinja -DCMAKE_TOOLCHAIN_FILE=${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake
```

`${VCPKG_ROOT}`: Follow their installation instruction and set this variable so that it points to where vcpkg is installed.

