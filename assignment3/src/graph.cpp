#include "graph.h"

#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <unordered_set>

Graph::Graph(std::vector<Vertex *> v) : numEdges(0) {
  for (Vertex *t : v) {
    addVertex(t);
  }
}

Graph::Graph() : Graph(std::vector<Vertex *>()) {}

void Graph::addVertex(Vertex *vertex) { v.push_back(vertex); }

void Graph::connect(Vertex *v1, Vertex *v2) {
  float dis = glm::length(v2->pos - v1->pos);
  Edge *e1 = new Edge(v2, dis);
  v1->edges.push_back(e1);
  numEdges++;
  Edge *e2 = new Edge(v1, dis);
  v2->edges.push_back(e2);
  numEdges++;
}

float Graph::h(Vertex *v1, Vertex *v2) {
  return glm::length(v2->pos - v1->pos);
}

std::deque<Vertex *> Graph::aStarSearch(Vertex *v1, Vertex *v2) {
  // std::cout << "A Star Search" << std::endl;
  return findShortestPath(v1, v2, true);
}
std::deque<Vertex *> Graph::uniformCostSearch(Vertex *v1, Vertex *v2) {
  // std::cout << "Uniform Cost Search" << std::endl;
  return findShortestPath(v1, v2, false);
}

std::deque<Vertex *> Graph::findShortestPath(Vertex *v1, Vertex *v2,
                                             bool useHeuristic) {
  int numExpanded = 0;
  std::unordered_set<Vertex *> closedSet;

  std::deque<Vertex *> openSet;
  openSet.push_front(v1);

  std::unordered_map<Vertex *, Vertex *> cameFrom;

  std::unordered_map<Vertex *, float> gScore;
  for (Vertex *t : v) {
    gScore[t] = std::numeric_limits<float>::infinity();
  }
  gScore[v1] = 0;

  // std::unordered_map<Vertex *, float> fScore;
  // for (Vertex *t : v) {
  //   fScore[t] = std::numeric_limits<float>::infinity();
  // }
  v1->f = 0;
  if (useHeuristic) v1->f += h(v1, v2);

  while (!openSet.empty()) {
    Vertex *current = openSet.front();
    for (Vertex *vv : openSet) {
      if (current == vv) continue;
      if (vv->f < current->f) current = vv;
    }

    numExpanded++;
    if (current == v2) {
      std::deque<Vertex *> ret({current});
      while (cameFrom.find(current) != cameFrom.end()) {
        current = cameFrom[current];
        ret.push_front(current);
      }
      // std::cout << "Found a solution" << std::endl;
      // std::cout << "Nodes: " << v.size() << std::endl;
      // std::cout << "Edges: " << numEdges << std::endl;
      // std::cout << "numExpanded = " << numExpanded << std::endl;
      // for (Vertex *s : ret) {
      //   printf("%g %g %g\n", s->pos.x, s->pos.y, s->pos.z);
      // }
      return ret;
    }

    openSet.erase(std::remove(openSet.begin(), openSet.end(), current));
    closedSet.emplace(current);

    for (Edge *neighbor : current->edges) {
      if (closedSet.find(neighbor->v) != closedSet.end()) {
        continue;
      }

      if (std::find(openSet.begin(), openSet.end(), neighbor->v) ==
          openSet.end()) {
        openSet.push_back(neighbor->v);
      }

      float tentativegScore = gScore[current] + neighbor->weight;
      if (tentativegScore >= gScore[neighbor->v]) {
        continue;
      }

      cameFrom[neighbor->v] = current;
      gScore[neighbor->v] = tentativegScore;
      neighbor->v->f = gScore[neighbor->v];
      if (useHeuristic) neighbor->v->f += h(neighbor->v, v2);
    }
  }

  return std::deque<Vertex *>();
}
