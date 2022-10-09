# Floyd–Steinberg dithering

Dithering implementation in C++ with `stb_image` and `glm`.

## Requires

  - [glm](https://github.com/g-truc/glm)
  - [cmake](https://cmake.org)

## Development

Setup the development environment with `cmake`.

```
cmake -S . -Bbuild -Dglm_DIR={GLM_SDK} -GNinja
```

`{GLM_SDK}`: This is the path where glm is installed on your system.

