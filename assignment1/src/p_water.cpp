#include "p_water.h"

#include "glm/gtc/constants.hpp"

WaterSystem::WaterSystem(int numParticles_, const char* modelFile)
    : ParticleSystem(numParticles_, modelFile) {}

void WaterSystem::update(float dt) {
  for (int i = 0; i < numParticles; i++) {
    lifetimes[i] += dt;
    if (lifetimes[i] > maxAge) {
      newParticle(i);
    }
    glm::vec3* p = &positions[i];
    glm::vec3* v = &velocities[i];
    *v += GRAVITY * dt;
    *p += *v * dt;
    if (p->z < modelRadius * modelScale.z) {
      p->z = modelRadius * modelScale.z;
      v->z *= -0.3;
      v->x *= 0.8;
      v->y *= 0.8;

      if (colors[i].g < 0.5 && rand01() < 0.3) {
        float color_add = 0.3 + 0.3 * rand01();
        colors[i] += glm::vec3(color_add, color_add, 0);
      } else {
        colors[i] = glm::vec3(0.2, 0.2, 0.9);
      }
    }
    if (p->x * p->x + p->y * p->y > 3) {
      newParticle(i);
    }
  }

  spawnNewParticles(dt);
}

WaterSystem::~WaterSystem() {}

glm::vec3 WaterSystem::initialParticlePosition(int i) {
  float t = 2 * glm::pi<float>() * rand01();
  float r = 0.05 * std::sqrt(rand01());
  float x = r * std::cos(t);
  float y = r * std::sin(t);
  float z = 0.1 * rand01() + 0.25;
  return glm::vec3(x, y, z);
}

glm::vec3 WaterSystem::initialParticleVelocity(int i) {
  float t = 2 * glm::pi<float>() * rand01();
  float r = 1 * std::sqrt(rand01());
  float x = r * std::cos(t);
  float y = r * std::sin(t);
  float z = 4 + 3 * rand01();
  return glm::vec3(x, y, z);
}

glm::vec3 WaterSystem::initialParticleColor(int i) {
  float rand_b = 0.8 + 0.2 * rand01();
  float rand_rg = 0.4 * rand01();
  return glm::vec3(rand_rg, rand_rg, rand_b);
}
