#include "vertex.h"

#include <limits>

Vertex::Vertex(glm::vec3 pos)
    : pos(pos),
      f(std::numeric_limits<float>::infinity()),
      edges(std::vector<Edge*>()) {}

Vertex::Vertex() : Vertex(glm::vec3()) {}
