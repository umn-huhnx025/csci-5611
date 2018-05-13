#include "p_snow.h"

SnowSystem::SnowSystem(int numParticles_, const char* modelFile)
    : ParticleSystem(numParticles_, modelFile) {}

SnowSystem::~SnowSystem() {}

void SnowSystem::update(float dt) {
  // printf("dt = %f\n", dt);
  // fflush(stdout);
  for (int i = 0; i < numParticles; i++) {
    // lifetimes[i] += dt;
    // if (lifetimes[i] > maxAge) {
    //   newParticle(i);
    // }
    positions[i] += velocities[i] * dt;

    if (positions[i].z < radius * modelScale.z) {
      newParticle(i);
    }
  }

  spawnNewParticles(dt);
}

glm::vec3 SnowSystem::initialParticlePosition(int i) {
  return glm::vec3(6 * rand01() - 3, 6 * rand01() - 3, 3);
}

glm::vec3 SnowSystem::initialParticleVelocity(int i) {
  return glm::vec3(0, 0, -0.3);
}

glm::vec3 SnowSystem::initialParticleColor(int i) {
  return glm::vec3(1., 1., 1.);
}
