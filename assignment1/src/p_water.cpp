#include "p_water.h"

#include "glm/gtc/constants.hpp"

WaterSystem::WaterSystem(int numParticles_, const char* modelFile)
    : ParticleSystem(numParticles_, modelFile) {}

void WaterSystem::update(float dt) {
  for (int i = 0; i < numParticles; i++) {
    // lifetimes[i] += dt;
    // if (lifetimes[i] > maxAge) {
    //   newParticle(i);
    // }
    glm::vec3* p = &positions[i];
    glm::vec3* v = &velocities[i];
    *v += GRAVITY * dt;
    *p += *v * dt;
    if (p->y < 0) {
      p->y = 0;
      // v->z *= 0.8;
      // v->x *= 0.8;
      v->y *= -0.3;

      if (colors[i].g < 0.5 && rand01() < 0.3) {
        float color_add = 0.3 + 0.3 * rand01();
        colors[i] += glm::vec3(color_add, color_add, 0);
      } else {
        colors[i] = glm::vec3(0.2, 0.2, 0.9);
      }
    }

    float xmin = -0.75;
    float xmax = -xmin;
    if ((p->x < xmin || p->x > xmax) && p->y < 0.2) {
      p->x = std::fmin(xmax, std::fmax(xmin, p->x));
      v->x *= -0.3;
    }

    for (int j = 0;j < numParticles; j++) {
      if (i==j)continue;
      if (glm::length(positions[i]-positions[j]) < 2*radius) {
        positions[i] = positions[j]+2*radius*glm::normalize(positions[i]-positions[j]);
      }
    }
  }

  spawnNewParticles(dt);
  updatePosData();
}

WaterSystem::~WaterSystem() {}

glm::vec3 WaterSystem::initialParticlePosition(int i) {
  float x = 1;
  float y = 0.6;
  float z = 0;
  return glm::vec3(x, y, z);
}

glm::vec3 WaterSystem::initialParticleVelocity(int i) {
  float t = 3 * 3.14159 / 4.;
  float r = 0.75 + 0.5 * rand01();
  float x = r * std::cos(t);
  float y = 2 + 1.5 * rand01();
  float z = 0;
  return glm::vec3(x, y, z);
}

glm::vec3 WaterSystem::initialParticleColor(int i) {
  float rand_b = 0.8 + 0.2 * rand01();
  float rand_rg = 0.4 * rand01();
  return glm::vec3(rand_rg, rand_rg, rand_b);
}
