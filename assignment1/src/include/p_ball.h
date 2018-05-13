#pragma once

#include "particle_system.h"

class BallSystem : public ParticleSystem {
 private:
  static const float MAX_AGE;  // in seconds

  // Defines the volume in which new particles can be created
  glm::vec3 initialParticlePosition(int i);

  // Defines the volume in which new particles can be created
  glm::vec3 initialParticleVelocity(int i);

  // Defines the color for a new particle
  glm::vec3 initialParticleColor(int i);

 public:
  BallSystem(int numParticles_ = 1,
             const char *modelFile = "models/sphere.obj");
  virtual ~BallSystem();

  void update(float dt);
};
