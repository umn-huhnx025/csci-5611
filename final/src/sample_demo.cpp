#include "sample_demo.h"

SampleFluidDemo::SampleFluidDemo() : SPHFluid() {
  maxParticles = 200;

  boxTop = 0.75;
  boxBottom = 0;
  boxLeft = -0.5;
  boxRight = 0.5;
  boxFront = 0.5;
  boxBack = -0.5;

  initVBO();
}

glm::vec3 SampleFluidDemo::initialParticlePosition(int i) {
  float x = -1.5 - 0.2 * rand01();
  float y = 1.2 + 0.2 * rand01();
  float z = -0.7;
  return glm::vec3(x, y, z);
}

glm::vec3 SampleFluidDemo::initialParticleVelocity(int i) {
  float x = 2 + 0.5 * rand01();
  float y = 2 + 1.5 * rand01();
  float z = 0.7;
  return glm::vec3(x, y, z);
}
