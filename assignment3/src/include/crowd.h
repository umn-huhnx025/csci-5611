#pragma once

#include "glm/glm.hpp"

#include "graph.h"

#define rand01() (rand() / (float)RAND_MAX)

class Crowd {
 public:
  int width, height;

  // Agents
  int numAgents;
  float rad;
  glm::vec3 *pos, *vel;
  Vertex **start, **goal;

  // Obstacles
  int numObstacles;
  glm::vec3 *oPos;
  float *oRad;

  // Roadmap
  Graph *g;
  int numRoadmapPoints;
  std::deque<Vertex *> *path;
  float *mapData;

  Crowd(int numAgents);

  void update(float dt);

  void makeRoadmap();

 private:
  float goalThresh = 0.05;
  float maxSpeed = 10;

  float edgeThresh = 20 * std::sqrt(2);

  // Tuning
  float sepCoef = 15;
  float sepMaxDist = 0.5;
  float cohCoef = 12;
  float cohMaxDist = 3;
  float alignCoef = 15;
  float alignMaxDist = 5;
  float pathCoef = 10;
  float pathSpeed = 30;

  Vertex *addVertex(glm::vec3 newPos);
  void addRandomPoints(int n);
  bool willCollide(glm::vec3 v1, glm::vec3 v2);

  void updateMapData();
};
