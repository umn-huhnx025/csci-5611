#include "p_ball.h"

BallSystem::BallSystem(int numParticles_, const char* modelFile)
    : ParticleSystem(numParticles_, modelFile) {
  modelScale = glm::vec3(1, 1, 1) * 1.f;
  spawnRate = 100;
}

BallSystem::~BallSystem() {}

void BallSystem::update(float dt) {
  for (int i = 0; i < numParticles; i++) {
    glm::vec3* p = &positions[i];
    glm::vec3* v = &velocities[i];
    float xMin = -3 + radius * modelScale.x;
    float xMax = 3 - radius * modelScale.x;
    float yMin = -3 + radius * modelScale.y;
    float yMax = 3 - radius * modelScale.y;

    *v += GRAVITY * dt;
    *p += *v * dt;

    if (v->z < 0.001 && p->z < 0.001 + radius) *v *= 0.9;

    if (p->z < radius) {
      p->z = radius;
      v->z *= -0.95;
    }
    if (p->x < xMin) {
      p->x = xMin;
      v->x *= -1;
    }
    if (p->x > xMax) {
      p->x = xMax;
      v->x *= -1;
    }
    if (p->y < yMin) {
      p->y = yMin;
      v->y *= -1;
    }
    if (p->y > yMax) {
      p->y = yMax;
      v->y *= -1;
    }
  }

  spawnNewParticles(dt);
}

glm::vec3 BallSystem::initialParticlePosition(int i) {
  return glm::vec3(0, 0, 2);
}

glm::vec3 BallSystem::initialParticleVelocity(int i) {
  return glm::vec3(0, 0, 0);
}

glm::vec3 BallSystem::initialParticleColor(int i) {
  return glm::vec3(1., 1., 1.);
}
