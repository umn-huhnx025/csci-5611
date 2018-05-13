#pragma once

#include <SDL.h>
#include <SDL_opengl.h>

#include "glm/glm.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <vector>

struct bubbleSound{
  float audioPosition = 0.0f;
  float radius;
};
extern std::vector<bubbleSound> bubbles;

short* audioBufferGeneration(bubbleSound audio, int bufferSize, float dt);

void bubbleGeneration(glm::vec3 velocity);
