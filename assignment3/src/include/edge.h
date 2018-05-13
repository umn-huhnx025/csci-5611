#pragma once

class Vertex;

class Edge {
 public:
  Vertex *v;
  float weight;

  Edge(Vertex *v, float weight);
};
