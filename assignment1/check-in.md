# Assignment 1 Check-in
### By Jon Huhn (huhnx025)

[Link to video](https://youtu.be/tZDLKjFQ1k8)

I implemented my solution using C++, OpenGL, and SDL2. I generate a sphere with some random initial color, position, and velocity. It bounces around inside a mostly invisible box. I render the bottom of the box, but no other faces.

Extra features I added were pressing the spacebar to inject additional velocity to each sphere, handling multiple spheres, and adding some camera motion (WASD to move the camera and arrow keys to adjust the angle). Camera controls aren't quite finished yet though.

Compile and run with

```
$ cmake . && make && ./assignment1
```

This requires CMake >= 3.1, which is included in the CSELabs machines.