#include "p_fire.h"

#include "glm/gtc/constants.hpp"

#include <cmath>

FireSystem::FireSystem(int numParticles_, const char* modelFile)
    : ParticleSystem(numParticles_, modelFile) {
  spawnRate = numParticles_ / 10.;
}

FireSystem::~FireSystem() {}

void FireSystem::update(float dt) {
  for (int i = 0; i < numParticles; i++) {
    lifetimes[i] += dt;
    if (lifetimes[i] > maxAge) {
      if (rand01() > 0.998)
        lifetimes[i] = rand01() * maxAge;
      else
        newParticle(i);
    }
    glm::vec3 p = positions[i];
    velocities[i] += glm::vec3(-p.x * 0.8, -p.y * 0.8, 0.05) * dt;
    positions[i] += velocities[i] * dt;

    colors[i] = glm::vec3(1, powf(lifetimes[i] / maxAge, 1.0), 0);
  }

  spawnNewParticles(dt);
}

glm::vec3 FireSystem::initialParticlePosition(int i) {
  float t = 3 * glm::pi<float>() * rand01();
  float r = std::sqrt(rand01()) * 0.2;
  float x = r * std::cos(t);
  float y = r * std::sin(t);
  float z = 0;
  return glm::vec3(x, y, z);
}

glm::vec3 FireSystem::initialParticleVelocity(int i) {
  return glm::vec3(0.6 * rand01() - 0.3, 0.6 * rand01() - 0.3, 0.5 * rand01());
}

glm::vec3 FireSystem::initialParticleColor(int i) {
  return glm::vec3(1, 0.5, 0);
}
