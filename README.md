# FluidBrushCG

A term project exploring a resolution-independent fluid brush for a
procedural marble texture, implemented with C++ and Vulkan.

## Project materials

- [Project instructions](AGENT.md)
- [Computer Graphics term project brief](<2110479 Computer Graphics — Term Project Template 2026 (1).pdf>)
- [Vulkan skeleton and planned features](skeleton.md)

## Build and run

The current application is a Vulkan 1.3 skeleton that opens a resizable
window and clears it every frame. Requirements: CMake 3.24+, the Vulkan SDK
1.3+, a C++20 compiler, and Git (GLFW is fetched at configure time).

```powershell
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
.\build\FluidBrushCG.exe
```

Options: `--validation` enables the validation layer (on by default in Debug
builds) and `--frames N` exits after `N` frames. See [skeleton.md](skeleton.md)
for details and for the toolchain flags used on the development machine.

## Quality checks

Run the repository integrity tests with Python 3.12 or newer:

```powershell
python -m unittest discover --start-directory tests --verbose
```
