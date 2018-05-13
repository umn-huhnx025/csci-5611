#pragma once

#include "sph_fluid.h"

// This is a sample class to show how you can make a slightly different demo
// than the base SPHFluid class. Notable things you might want to override
// include tuning parameters, initial condition functions, and collision
// resolution. Feel free to make any other functions virtual in the base class
// if you want to override them.
class SampleFluidDemo : public SPHFluid {
 public:
  SampleFluidDemo();

 private:
  glm::vec3 initialParticlePosition(int i);
  glm::vec3 initialParticleVelocity(int i);
};
