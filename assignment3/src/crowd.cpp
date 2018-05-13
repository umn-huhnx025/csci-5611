#include "crowd.h"

#include <algorithm>
#include <iostream>

Crowd::Crowd(int numAgents)
    : width(20),
      height(20),
      numAgents(numAgents),
      rad(0.5),
      numObstacles(0),
      g(new Graph()),
      numRoadmapPoints(0) {
  pos = new glm::vec3[numAgents];
  vel = new glm::vec3[numAgents];
  start = new Vertex *[numAgents];
  goal = new Vertex *[numAgents];
  path = new std::deque<Vertex *>[numAgents];

  oPos = new glm::vec3[numObstacles];
  oRad = new float[numObstacles];

  // Random
  for (int i = 0; i < numObstacles; i++) {
    if (i == 0) {
      oPos[0] = glm::vec3(10, 0, 10);
      oRad[0] = 2;
    } else {
      oRad[i] = 1;  // + rand01() * 0.5;
      float x = rand01() * (height - 2 * oRad[i]) + oRad[i];
      float z = rand01() * (width - 2 * oRad[i]) + oRad[i];
      oPos[i] = glm::vec3(x, 0, z);
      for (int j = 0; j < i; j++) {
        if (glm::length(oPos[i] - oPos[j]) < oRad[i] + oRad[j]) {
          oPos[i] =
              oPos[j] + (oRad[i] + oRad[j]) * glm::normalize(oPos[i] - oPos[j]);
        }
      }
    }
  }

  // Row in front of goal
  // for (int i = 0; i < numObstacles; i++) {
  //   oRad[i] = 0.5;
  //   if (i < 9) {
  //     oPos[i] = glm::vec3(height - 0.5 - i, 0, width - 2.5);
  //   } else {
  //     oPos[i] = glm::vec3(height - 2.5 - i, 0, width - 2.5);
  //   }
  // }

  for (int i = 0; i < numAgents; i++) {
    // Random
    // float x = rand01() * (height - 2 * rad) + rad;
    // float z = rand01() * (width - 2 * rad) + rad;
    // glm::vec3 newPos(x, 0, z);
    // for (int j = 0; j < i; j++) {
    //   if (glm::length(newPos - start[j]->pos) < 2 * rad) {
    //     newPos =
    //         start[j]->pos + 2 * rad * glm::normalize(newPos - start[j]->pos);
    //   }
    // }
    // start[i] = addVertex(newPos);
    // goal[i] = addVertex(newPos);

    // Along edges
    switch (i % 4) {
      case 0:
        start[i] = addVertex(glm::vec3(0.5, 0, 2 * i / 2. + 2));
        goal[i] =
            addVertex(glm::vec3(height - start[i]->pos.x, 0, start[i]->pos.z));
        break;
      case 1:
        start[i] = addVertex(glm::vec3(2 * i / 2. + 1, 0, 0.5));
        goal[i] =
            addVertex(glm::vec3(start[i]->pos.x, 0, width - start[i]->pos.z));
        break;
      case 2:
        start[i] = addVertex(glm::vec3(height - 0.5, 0, width - 2 * i / 2.));
        goal[i] = addVertex(glm::vec3(0.5, 0, 2 * i / 2.));
        break;
      case 3:
        start[i] = addVertex(glm::vec3(2 * i / 2. - 1, 0, width - 0.5));
        goal[i] =
            addVertex(glm::vec3(start[i]->pos.x, 0, width - start[i]->pos.z));
        break;
    }

    // Along one edge
    // start[i] = addVertex(glm::vec3(1.1 * i + 5, 0, 0.5));
    // goal[i] = addVertex(glm::vec3(2 * i + 0.5, 0, width - start[i]->pos.z));

    pos[i] = start[i]->pos;
    vel[i] = glm::vec3();
  }

  makeRoadmap();
  updateMapData();
}

void Crowd::update(float dt) {
  for (int i = 0; i < numAgents; i++) {
    if (pathCoef > 0) {
      if (path[i].size() < 2) {
        continue;
      }

      // if (willCollide(pos[i], path[i][1]->pos)) {
      //   printf("Agent %d got off course. Finding new path...\n", i);
      //   path[i] = std::deque<Vertex *>();
      //   Vertex *newPos = addVertex(pos[i]);
      //   int n = 1;
      //   while (true) {
      //     path[i] = g->findShortestPath(newPos, goal[i]);
      //     if (!path[i].empty()) break;

      //     addRandomPoints(n);
      //     n *= 2;

      //     if (n > 1000) {
      //       // printf("I give up :(\n");
      //       break;
      //     }
      //   }
      //   // printf("Done\n");
      // }

      // Smooth path
      while (true) {
        if (path[i].size() < 3) break;
        if (willCollide(pos[i], path[i][2]->pos)) break;
        path[i].erase(path[i].begin() + 1);
      }
    }

    // Boids separation
    glm::vec3 sepForce;
    for (int j = 0; j < numAgents; j++) {
      if (i == j) continue;
      glm::vec3 v = pos[i] - pos[j];
      float dist = glm::length(v) - 2 * rad;
      if (dist < sepMaxDist) {
        glm::vec3 n = sepCoef / dist * glm::normalize(v);
        if (std::isnan(n.x) || std::isnan(n.y) || std::isnan(n.z)) continue;
        sepForce += n;
      }
    }

    for (int j = 0; j < numObstacles; j++) {
      glm::vec3 v = pos[i] - oPos[j];
      float dist = glm::length(v) - (rad + oRad[j]);
      if (dist < sepMaxDist) {
        glm::vec3 n = sepCoef / dist * glm::normalize(v);
        if (std::isnan(n.x) || std::isnan(n.y) || std::isnan(n.z)) continue;
        sepForce += n;
      }
    }

    // Boids Cohesion
    glm::vec3 cohForce;
    glm::vec3 centerOfMass;
    int count = 0;
    for (int j = 0; j < numAgents; j++) {
      if (i == j) continue;
      if (glm::length(pos[i] - pos[j]) < cohMaxDist) {
        count++;
        centerOfMass += pos[j];
      }
    }
    if (count) {
      centerOfMass /= (float)count;
      cohForce = cohCoef * (centerOfMass - pos[i]);
    }

    // Boids alignment
    glm::vec3 alignForce;
    glm::vec3 avgVel;
    count = 0;
    for (int j = 0; j < numAgents; j++) {
      if (i == j) continue;
      if (glm::length(pos[i] - pos[j]) < alignMaxDist) {
        count++;
        avgVel += vel[j];
      }
    }
    if (count) alignForce = alignCoef * avgVel / (float)count;

    glm::vec3 pathForce;
    if (pathCoef > 0) {
      // Path-following force
      pathForce =
          pathCoef *
          (pathSpeed * glm::normalize(path[i][1]->pos - pos[i]) - vel[i]);
    }

    glm::vec3 force = pathForce + sepForce + cohForce + alignForce;

    vel[i] = force * dt;

    // Cap speed
    if (glm::length(vel[i]) > maxSpeed)
      vel[i] = maxSpeed * glm::normalize(vel[i]);

    pos[i] += dt * vel[i];

    // printf("pos[%d] = %f %f %f\n", i, pos[i].x, pos[i].y, pos[i].z);

    // Fix collisions
    // With other agents
    for (int j = 0; j < numAgents; j++) {
      if (i == j) continue;
      if (glm::length(pos[i] - pos[j]) < 2 * rad) {
        pos[i] = pos[j] + 2 * rad * glm::normalize(pos[i] - pos[j]);
      }
    }
    // With obstacles
    for (int j = 0; j < numObstacles; j++) {
      if (glm::length(pos[i] - oPos[j]) < rad + oRad[j]) {
        pos[i] = oPos[j] + (rad + oRad[j]) * glm::normalize(pos[i] - oPos[j]);
      }
    }
    // With walls
    pos[i].x = std::max(rad, std::min(pos[i].x, height - rad));
    pos[i].z = std::max(rad, std::min(pos[i].z, width - rad));

    if (pathCoef > 0) {
      // If we're done on this leg of the path, move on to the next one
      if (glm::length(path[i][1]->pos - pos[i]) < goalThresh) {
        path[i].pop_front();
        pos[i] = path[i][0]->pos;
      }
    }

    // printf("pathForce = %f %f %f\n", pathForce.x, pathForce.y, pathForce.z);
    // printf("corForce = %f %f %f\n", corForce.x, corForce.y, corForce.z);
    // printf("sepForce = %f %f %f\n", sepForce.x, sepForce.y, sepForce.z);
    // printf("cohForce = %f %f %f\n", cohForce.x, cohForce.y, cohForce.z);
    // printf("alignForce = %f %f %f\n", alignForce.x, alignForce.y,
    //        alignForce.z);
    // printf("force = %f %f %f\n", force.x, force.y, force.z);
    // printf("vel[%d] = %f %f %f\n", i, vel[i].x, vel[i].y, vel[i].z);
  }
}

void Crowd::makeRoadmap() {
  numRoadmapPoints += numAgents * 2;
  addRandomPoints(numRoadmapPoints - 2 * numAgents);

  for (int i = 0; i < numAgents; i++) {
    path[i] = g->aStarSearch(start[i], goal[i]);
    for (int j = 0; j < 10 && path[i].empty(); j++) {
      // std::cerr << "No path found. Doubling number of nodes to "
      //           << numRoadmapPoints * 2 << std::endl;
      addRandomPoints(numRoadmapPoints);
      numRoadmapPoints *= 2;
      path[i] = g->aStarSearch(start[i], goal[i]);
    }
    if (path[i].empty()) {
      printf("I give up :(\n");
      path[i] = {start[i], goal[i]};
    }
  }

  // printf("Graph vertices:\n");
  // for (int i = 0; i < g->v.size(); i++) {
  //   printf("%f %f %f\n", g->v[i]->pos.x, g->v[i]->pos.y, g->v[i]->pos.z);
  // }

  printf("Graph has %d vertices, %d edges\n", (int)g->v.size(), g->numEdges);
}

Vertex *Crowd::addVertex(glm::vec3 newPos) {
  Vertex *v1 = new Vertex(newPos);
  g->addVertex(v1);
  for (Vertex *v2 : g->v) {
    if (v1 == v2) continue;
    if (glm::length(v1->pos - v2->pos) < edgeThresh) {
      if (willCollide(v1->pos, v2->pos)) continue;
      g->connect(v1, v2);
    }
  }
  return v1;
}

void Crowd::addRandomPoints(int n) {
  // printf("here\n");
  for (int i = 0; i < n;) {
    bool skip = false;
    float x = rand01() * (height - 2 * rad) + rad;
    float z = rand01() * (width - 2 * rad) + rad;
    glm::vec3 p(x, 0, z);
    for (int j = 0; j < numObstacles; j++) {
      if (glm::length(p - oPos[j]) < rad + oRad[j]) {
        skip = true;
        break;
      }
    }
    if (skip) continue;
    addVertex(p);
    i++;
  }
}

bool Crowd::willCollide(glm::vec3 v1, glm::vec3 v2) {
  for (int k = 0; k < numObstacles; k++) {
    float r = oRad[k] + rad;
    glm::vec3 d = v2 - v1;
    glm::vec3 f = v1 - oPos[k];
    float a = glm::dot(d, d);
    float b = 2 * glm::dot(f, d);
    float c = glm::dot(f, f) - r * r;
    float disc = b * b - 4 * a * c;
    if (disc < 0) continue;
    disc = std::sqrt(disc);
    float t1 = (-b - disc) / (2 * a);
    float t2 = (-b + disc) / (2 * a);
    if ((t1 >= 0 && t1 <= 1) || (t2 >= 0 && t2 <= 1)) {
      return true;
    }
  }
  return false;
}

void Crowd::updateMapData() {
  std::vector<float> data;

  // Edges
  for (Vertex *v : g->v) {
    for (Edge *e : v->edges) {
      data.push_back(v->pos.x);
      data.push_back(v->pos.y);
      data.push_back(v->pos.z);
      data.push_back(e->v->pos.x);
      data.push_back(e->v->pos.y);
      data.push_back(e->v->pos.z);
    }
  }

  // printf("Vertices start at %lu\n", data.size());

  // Vertices
  for (unsigned i = 0; i < g->v.size(); i++) {
    data.push_back(g->v[i]->pos.x);
    data.push_back(g->v[i]->pos.y);
    data.push_back(g->v[i]->pos.z);
  }

  mapData = new float[data.size()];
  std::copy(data.begin(), data.end(), mapData);
  // for (long unsigned i = 0; i < data.size(); i++) {
  //   printf("data[%d] = %f\n", i, mapData[i]);
  // }
}
