#pragma once

#include <vector>

#include "glm/glm.hpp"

#include "edge.h"

class Vertex {
 public:
  glm::vec3 pos;
  float f;
  std::vector<Edge *> edges;

  Vertex(glm::vec3 pos);
  Vertex();
};
