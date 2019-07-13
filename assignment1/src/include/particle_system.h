#pragma once

#include <vector>

#include "glm/glm.hpp"

// Library to load obj files
// https://github.com/syoyo/tinyobjloader
#include "tiny_obj_loader.h"

inline float rand01() { return rand() / (float)RAND_MAX; }

class ParticleSystem {
 protected:
  glm::vec3 *positions;
  glm::vec3 *colors;
  glm::vec3 *velocities;
  float *lifetimes;
  // float *transparencies;
  int numParticles;
  int maxParticles;
  float spawnRate;

  // Error rate to carry over when creating new particles
  float spawnError;

  glm::vec3 modelScale;
  float maxAge;
  std::vector<tinyobj::real_t> modelData;
  float modelRadius;

  static const glm::vec3 GRAVITY;

  // Defines the volume in which new particles can be created
  virtual glm::vec3 initialParticlePosition(int i);

  // Defines the volume in which new particles can be created
  virtual glm::vec3 initialParticleVelocity(int i);

  // Defines the color for a new particle
  virtual glm::vec3 initialParticleColor(int i);

  // Creates a new particle at index i
  virtual void newParticle(int i);

  virtual void spawnNewParticles(float dt);

 public:
  ParticleSystem(int numParticles_, const char *modelFile);
  virtual ~ParticleSystem();

  virtual void update(float dt);
  int NumParticles() { return numParticles; }
  glm::vec3 Position(int i) { return positions[i]; }
  glm::vec3 Color(int i) { return colors[i]; }
  glm::vec3 Scale() { return modelScale; }
  glm::vec3 Velocity(int i) { return velocities[i]; }
  void Velocity(int i, glm::vec3 newV) { velocities[i] = newV; }
  std::vector<tinyobj::real_t> Model() { return modelData; }

  static std::vector<tinyobj::real_t> loadModel(const char *filename);
};
