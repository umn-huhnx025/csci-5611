#pragma once

#include <deque>

#include "path.h"
#include "vertex.h"

class Graph {
 public:
  std::vector<Vertex *> v;
  int numEdges;

  Graph(std::vector<Vertex *> v);
  Graph();

  void addVertex(Vertex *vertex);
  void connect(Vertex *v1, Vertex *v2);
  float h(Vertex *v1, Vertex *v2);

  std::deque<Vertex *> findShortestPath(Vertex *v1, Vertex *v2,
                                        bool useHeuristic);
  std::deque<Vertex *> aStarSearch(Vertex *v1, Vertex *v2);
  std::deque<Vertex *> uniformCostSearch(Vertex *v1, Vertex *v2);
};
