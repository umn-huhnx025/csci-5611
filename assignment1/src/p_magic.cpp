#include "p_magic.h"

#include "glm/gtc/constants.hpp"

#include <cmath>

// float MagicSystem::tm = 0;

MagicSystem::MagicSystem(int numParticles_, const char* modelFile)
    : ParticleSystem(numParticles_, modelFile) {
  spawnRate = numParticles_ / 10.;
}

MagicSystem::~MagicSystem() {}

void MagicSystem::update(float dt) {
  float tm = 0.5;
  glm::vec3 c1 = glm::vec3(0, 0.8, 0);
  glm::vec3 c2 = glm::vec3(0.2, 0.2, 0.8);
  glm::vec3 c3 = glm::vec3(0.8, 0.0, 0.8);

  spawnNewParticles(dt);
  for (int i = 0; i < numParticles; i++) {
    float t = lifetimes[i] / maxAge;
    lifetimes[i] += dt;
    if (lifetimes[i] > maxAge) {
      newParticle(i);
    }

    glm::vec3* p = &positions[i];
    glm::vec3* v = &velocities[i];

    // *v += glm::normalize(*p) * glm::vec3(0.1, 0.1, 0.1) * dt;
    *v += glm::vec3(p->x * 0.25, p->y * 0.25, 0.25) * dt;
    *p += *v * dt;

    // Lerp color from green -> blue -> magenta
    if (t < tm)
      colors[i] = c1 * (1 - t / tm) + c2 * (t / tm);
    else
      colors[i] = c2 * (1 - (t - tm) / (1 - tm)) + c3 * (t - tm) / (1 - tm);
  }
}

glm::vec3 MagicSystem::initialParticlePosition(int i) {
  float t = 2 * glm::pi<float>() * rand01();
  float r = std::sqrt(rand01()) * 0.25;
  float x = r * std::cos(t);
  float y = r * std::sin(t);
  float z = 0;
  return glm::vec3(x, y, z);
}

glm::vec3 MagicSystem::initialParticleVelocity(int i) {
  glm::vec3* p = &positions[i];
  return glm::vec3(-p->y, p->x, 0.2 + 0.2 * rand01());
}

glm::vec3 MagicSystem::initialParticleColor(int i) {
  return glm::vec3(0, 0.8, 0);
}
