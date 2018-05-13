#pragma once

#include "glm/glm.hpp"

#include <unordered_map>
#include <vector>

#include "spring_system.h"

// Return a random number [0, 1]
inline float rand01() { return rand() / (float)RAND_MAX; }

struct NeighborSet {
  // Save space for as many neighbors as we expect
  // We might be able to restrict this a bit if we're clever about resizing it
  // when it fills up.
  int n[400];
  int size;
  NeighborSet() : size(0) {}
};

class SPHFluid {
 public:
  static const int BOX_VERTICES;

  SPHFluid(SpringSystem *ss = nullptr, bool heat = false);
  virtual ~SPHFluid();

  virtual void update(float dt);

  // Getters
  int NumParticles() { return numParticles; }
  float *VboData() { return vboData; }
  glm::vec3 Position(int i) { return pos[i]; }
  glm::vec3 Velocity(int i) { return vel[i]; }
  glm::vec3 Scale() { return glm::vec3(r, r, r); }
  float Radius() { return r; }
  int vboSize();

 protected:
  float dt;          // Timestep
  int numParticles;  // Current number of particles
  int maxParticles;  // Max number of particles allowed
  glm::vec3 *pos;    // 3D particle position
  glm::vec3 *ppos;   // Buffer of previous positions to derive velocity
  glm::vec3 *vel;    // 3D particle velocity
  glm::vec3 *col;
  float *heat;  // temperature of particles

  // x, y, z position data to send to the VBO
  float *vboData;

  // Parallel vectors of i, j indices and rest lengths to represent springs
  std::vector<int> sp1, sp2;
  std::vector<float> spL;

  // Error rate to carry over when creating new particles
  float spawnError;
  float spawnRate;

  // Number of simulation iterations per rendering iteration
  int simSteps;

  NeighborSet n;  // Placeholder for during simulation, only need one at a time

  // Tuning parameters

  // Interaction radius
  float h;

  // Radius of each particle
  float r;

  // Viscosity (higher is more viscous)
  float sig;  // Linear
  float bet;  // Quadratic

  // Adjust nearby springs
  float g;
  float a;
  float ks;  // Spring constant

  // Pressure constants
  float k;
  float knear;
  float p0;

  // End tuning parameters

  static const glm::vec3 GRAVITY;

  float boxTop;
  float boxBottom;
  float boxLeft;
  float boxRight;
  float boxFront;
  float boxBack;
  float boxWallWidth;

  // Hash grid spatial data structure to accelerate neighbor-finding
  // Based off of CUDA implementation given by Simon Green
  // http://developer.download.nvidia.com/assets/cuda/files/particles.pdf
  float gridRes;
  glm::ivec3 gridSize;
  glm::vec3 worldOrigin;
  glm::ivec2 *particleHash;
  int *cellStart;
  int maxCellId;

  SpringSystem *ss;  // Associated cloth system
  bool useHeat;

  // Simulation functions
  void applyViscosity();
  void updateSprings();
  void doubleDensityRelaxation();
  virtual void resolveCollisions();
  void clothInteraction();
  glm::vec3 triangleSphereCollisionPoint(int v1, int v2, int v3, int i);
  void transferHeat();

  // Get the indices of particles within h of p
  void getNeighbors(int i, NeighborSet *n, bool pairwise = false);

  // Spatial hash grid functions
  glm::ivec3 getGridPos(glm::vec3 *p);
  int getCellId(glm::ivec3 *gridPos);
  void clampGridPos(glm::ivec3 *gridPos);
  void findCellStart();
  void makeGrid();

  // Defines the volume in which new particles can be created
  virtual glm::vec3 initialParticlePosition(int i);

  // Defines the volume in which new particles can be created
  virtual glm::vec3 initialParticleVelocity(int i);

  // Defines the temperature range at which new particles can be created
  virtual float initialParticleHeat(int i);

  // Creates a new particle at index i
  void newParticle(int i);

  // Create new particles at the end of each time step
  void spawnNewParticles();

  // Update data for box VBO each time step
  void initVBO();
  void updateVBO();
};
