#include "s_flag.h"

Flag::Flag() : SpringSystem(40, 30) {
  restLen = 0.08;
  wind = glm::vec3(2, 0, 2);
  sphereR = 0.0;
  // spherePos = glm::vec3(0, 0, 0);
  gravity = glm::vec3(0, 0, 0);
  setInitialPositions();
  setTextureCoords();
}

void Flag::setInitialPositions() {
  float totalWidth = restLen * width;
  pos = new glm::vec3[numNodes];
  for (int i = 0; i < width + 1; i++) {
    pos[i * (height + 1)] =
        glm::vec3(-0.5 * totalWidth + i * restLen + 2, 1.5, -2.5);
    for (int j = 1; j < height + 1; j++) {
      int ij = i * (height + 1) + j;
      pos[ij] = pos[ij - 1] - glm::vec3(0, restLen, 0);
    }
  }
}

void Flag::setTextureCoords() {
  for (int i = 0; i < numNodes; i++) {
    int u = i / (height + 1);
    int v = i % (height + 1);
    tex[i] = glm::vec2(u / (float)width, v / (float)height);
  }
}

void Flag::fixNodes() {
  for (int i = 0; i < height + 1; i++) {
    vel2[i] = glm::vec3();
  }
}

void Flag::detectCollisions() {}
