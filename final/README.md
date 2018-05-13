# CSCI 5611 Final Project

Particle-based fluid simulation based on [Clavet et al. 2005](http://www.ligum.umontreal.ca/Clavet-2005-PVFS/pvfs.pdf).

## Compile and Run

```
$ mkdir build
$ cd build
$ cmake ..
$ make && ./final
```

### Camera Controls

- Move with WASD
- Look around with arrow keys

## Tools Used

- C++
- CMake
- OpenGL 3+
- SDL2
- GLM vector math library
- [tinyobjloader](https://github.com/syoyo/tinyobjloader) to load .obj models
