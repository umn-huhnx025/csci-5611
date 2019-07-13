#pragma once

#include <SDL.h>

#include "glm/glm.hpp"

class SpringSystem {
 public:
  int width, height, numVertices;
  float *vertices, sphereR;
  glm::vec3 spherePos;

  SpringSystem(int w, int h);
  virtual ~SpringSystem();

  virtual void moveBall(const Uint8 *keyState);

  void update(float dt);

 protected:
  int numNodes, numSprings, simSteps;
  float k, kv, restLen, sphereSpeed;
  glm::vec3 *pos, *vel1, *vel2, *vmid, *norm, *drag, wind, gravity;
  glm::vec2 *tex;
  static const float DRAG_COEF;

  virtual void initBuffers();
  virtual void setInitialPositions();
  virtual void setTextureCoords();

  virtual glm::vec3 springForce(int ij1, int ij2, float dt);
  virtual void fixNodes();
  virtual void detectCollisions();

  virtual void updateDrag();
  virtual void updateVertices();
  virtual void updateNormals();
};
