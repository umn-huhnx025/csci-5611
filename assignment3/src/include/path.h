#pragma once

#include <vector>

#include "edge.h"

class Path {
 public:
  std::vector<Edge *> edges;
  Vertex *prev, *next;

  Path();
};
