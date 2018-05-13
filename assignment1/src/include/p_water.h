#pragma once

#include "particle_system.h"

class WaterSystem : public ParticleSystem {
 private:
  // Defines the volume in which new particles can be created
  glm::vec3 initialParticlePosition(int i);

  // Defines the volume in which new particles can be created
  glm::vec3 initialParticleVelocity(int i);

  // Defines the color for a new particle
  glm::vec3 initialParticleColor(int i);

 public:
  WaterSystem(int NumParticles_ = 1000,
              const char* modelFile = "models/floor.obj");
  virtual ~WaterSystem();

  void update(float dt);
};
