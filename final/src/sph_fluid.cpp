#include "sph_fluid.h"
#include "sound.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>

#include <glm/gtx/projection.hpp>

// Enable spatial data structure
#ifndef GRID
#define GRID
#endif

// #define DEBUG

const glm::vec3 SPHFluid::GRAVITY(0, -9.8, 0);
const int SPHFluid::BOX_VERTICES = 24;

SPHFluid::SPHFluid(SpringSystem *ss, bool heat)
    : numParticles(0),
      maxParticles(1000),
      pos(new glm::vec3[maxParticles]),
      ppos(new glm::vec3[maxParticles]),
      vel(new glm::vec3[maxParticles]),
      col(new glm::vec3[maxParticles]),
      heat(new float[maxParticles]),
      vboData(new float[4 * (BOX_VERTICES + maxParticles)]),
      spawnError(0.),
      spawnRate(maxParticles / 1.),
      simSteps(1),
      // Tuning parameters
      h(0.2),
      r(0.02),
      sig(2),
      bet(0),
      g(0.1),
      a(0.3),
      ks(0.3),
      k(8),
      knear(20),
      p0(10),
      // End tuning parameters
      boxTop(1),
      boxBottom(0),
      boxLeft(-0.5),
      boxRight(0.5),
      boxFront(0.5),
      boxBack(-0.5),
      boxWallWidth(0.25),
      gridRes(h),
      gridSize(glm::vec3((boxRight - boxLeft) / gridRes,
                         (boxTop - boxBottom) / gridRes, 1)),
      worldOrigin(glm::vec3(boxLeft, boxBottom, 0)),
      particleHash(new glm::ivec2[maxParticles]),
      cellStart(new int[maxParticles]),
      maxCellId(gridSize.x * gridSize.y * gridSize.z),
      ss(ss),
      useHeat(heat) {
  initVBO();
}

SPHFluid::~SPHFluid() {
  delete[] pos;
  delete[] ppos;
  delete[] vel;
  delete[] vboData;
  delete[] particleHash;
  delete[] cellStart;
}

void SPHFluid::newParticle(int i) {
  pos[i] = initialParticlePosition(i);
  vel[i] = initialParticleVelocity(i);
  heat[i] = initialParticleHeat(i);
}

void SPHFluid::spawnNewParticles() {
  if (numParticles < maxParticles) {
    float numNewParticlesExact = spawnRate * dt;
    int numNewParticles = int(numNewParticlesExact) + int(spawnError);
    spawnError += std::fmod(numNewParticlesExact, 1.) - int(spawnError);
    for (int i = 0; i < numNewParticles && numParticles < maxParticles; i++) {
      newParticle(numParticles);
      numParticles++;
    }
  }
}

void SPHFluid::update(float delta) {
  if (ss) ss->update(delta);
  dt = delta / (float)simSteps;
  NeighborSet n;
  for (int waka = 0; waka < simSteps; waka++) {
    // Apply gravity
    for (int i = 0; i < numParticles; i++) {
      vel[i] += dt * GRAVITY;
    }

    applyViscosity();

    if (useHeat) transferHeat();

    for (int i = 0; i < numParticles; i++) {
      // Save previous positions
      ppos[i] = pos[i];
      // Go to predicted position
      pos[i] += dt * vel[i];
    }

    updateSprings();

    doubleDensityRelaxation();

    resolveCollisions();

    clothInteraction();

    // Compute next velocity
    for (int i = 0; i < numParticles; i++) {
      vel[i] = (pos[i] - ppos[i]) / dt;
#ifdef DEBUG
      if (vel[i].y > 8) {
        printf("ppos[%d] = %f %f %f\n", i, ppos[i].x, ppos[i].y, ppos[i].z);
        printf(" pos[%d] = %f %f %f\n", i, pos[i].x, pos[i].y, pos[i].z);
        printf(" vel[%d].y = %f\n", i, vel[i].y);
      }
#endif
    }

    spawnNewParticles();
  }
  updateVBO();
}

void SPHFluid::applyViscosity() {
  makeGrid();

  // Apply viscosity
  glm::vec3 rij, I;
  for (int i = 0; i < numParticles; i++) {
    getNeighbors(i, &n, true);
    for (int ii = 0; ii < n.size; ii++) {
      int j = n.n[ii];
      float dist = glm::length(pos[i] - pos[j]);
      float q = dist / h;
      // Inward radial velocity
      rij = glm::normalize(pos[i] - pos[j]);
      float u = glm::dot(vel[i] - vel[j], rij);
      if (u > 0) {
        // Linear and quadratic impulses
        I = dt * (1 - q) * (sig * u + bet * u * u) * rij;

        vel[i] -= I / 2.f;
        vel[j] += I / 2.f;
      }
    }
  }
}

void SPHFluid::updateSprings() {
  makeGrid();

  // Adjust springs
  std::pair<int, int> key;
  float L, d;
  for (int i = 0; i < numParticles; i++) {
    getNeighbors(i, &n, true);
    for (int ii = 0; ii < n.size; ii++) {
      int j = n.n[ii];
      float dist = glm::length(pos[i] - pos[j]);

      // Insert new spring if it doesn't exist
      bool exists = true;
      auto iLo = std::lower_bound(sp1.begin(), sp1.end(), i);
      auto iHi = std::upper_bound(sp1.begin(), sp1.end(), i);
      auto jLo = sp2.begin() + (iLo - sp1.begin());
      auto jHi = sp2.begin() + (iHi - sp1.begin());
      auto jPos = std::lower_bound(jLo, jHi, j);
      int offset = jPos - sp2.begin();
      if (iLo == sp1.end() || jPos == jHi) exists = false;
      if (!exists) {
        sp1.insert(iLo, i);
        sp2.insert(jPos, j);
        spL.insert(spL.begin() + offset, h);
      }

      float &Lij = spL[offset];
      // Assuming Lij = L from the paper
      L = Lij;

      // Tolerable deformation = yield ratio * rest length
      d = g * Lij;
      if (dist > L + d) {
        Lij += dt * a * (dist - L - d);
      } else if (dist < L - d) {
        Lij -= dt * a * (L - d - dist);
      }
    }
  }
  for (auto it = spL.begin(); it != spL.end();) {
    if (*it > h) {
      int offset = it - spL.begin();
      sp1.erase(sp1.begin() + offset);
      sp2.erase(sp2.begin() + offset);
      it = spL.erase(it);
    } else {
      it++;
    }
  }

  // Apply spring displacements
  glm::vec3 v, D;
  for (int t = 0; t < spL.size(); t++) {
    int i = sp1[t];
    int j = sp2[t];
    v = pos[j] - pos[i];
    float dist = glm::length(v);
    D = dt * dt * ks * (1 - spL[t] / h) * (spL[t] - dist) * glm::normalize(v);

    pos[i] += D / -2.f;
    pos[j] += D / 2.f;
  }
}

void SPHFluid::doubleDensityRelaxation() {
  makeGrid();

  // Double density relaxation
  float p, pn;
  for (int i = 0; i < numParticles; i++) {
    p = 0;
    pn = 0;

    // Compute density and near-density
    getNeighbors(i, &n);
    for (int ii = 0; ii < n.size; ii++) {
      int j = n.n[ii];
      float dist = glm::length(pos[i] - pos[j]);
      float q = dist / h;
      if (q < 1) {
        p += powf(1 - q, 2);
        pn += powf(1 - q, 3);
      }
    }

    // Compute pressure and near-pressure
    float pres = k * (p - p0);
    float presn = knear * pn;

    glm::vec3 dx, D;
    for (int ii = 0; ii < n.size; ii++) {
      int j = n.n[ii];
      float dist = glm::length(pos[i] - pos[j]);
      float q = dist / h;
      if (q < 1) {
        // Apply displacements
        D = dt * dt * (pres * (1 - q) + presn * (1 - q) * (1 - q)) *
            glm::normalize(pos[j] - pos[i]);
        pos[j] += D / 2.f;
        dx -= D / 2.f;
      }
    }
    pos[i] += dx;
  }
}

void SPHFluid::resolveCollisions() {
  // Resolve collisions
  glm::vec3 norm, vn, vt, v, I;
  float mu = 0.05;
  for (int i = 0; i < numParticles; i++) {
    if (pos[i].y < boxBottom) {
      v = (pos[i] - ppos[i]) / dt;
      norm = glm::vec3(0, 1, 0);
      vn = glm::dot(v, norm) * norm;
      vt = v - vn;
      I = vn - mu * vt;
      pos[i] += dt * I;
      bubbleGeneration(vel[i]);
    }
    if (pos[i].x < boxLeft && pos[i].x > boxLeft - boxWallWidth &&
        pos[i].y < boxTop) {
      v = (pos[i] - ppos[i]) / dt;
      norm = glm::vec3(1, 0, 0);
      vn = glm::dot(v, norm) * norm;
      vt = v - vn;
      I = vn - mu * vt;
      pos[i] += dt * I;
      bubbleGeneration(vel[i]);
    }
    if (pos[i].x > boxRight && pos[i].x < boxRight + boxWallWidth &&
        pos[i].y < boxTop) {
      v = (pos[i] - ppos[i]) / dt;
      norm = glm::vec3(-1, 0, 0);
      vn = glm::dot(v, norm) * norm;
      vt = v - vn;
      I = vn - mu * vt;
      pos[i] += dt * I;
      bubbleGeneration(vel[i]);
    }
    if (pos[i].z > boxFront && pos[i].z < boxFront + boxWallWidth &&
        pos[i].y < boxTop) {
      v = (pos[i] - ppos[i]) / dt;
      norm = glm::vec3(0, 0, -1);
      vn = glm::dot(v, norm) * norm;
      vt = v - vn;
      I = vn - mu * vt;
      pos[i] += dt * I;
      bubbleGeneration(vel[i]);
    }
    if (pos[i].z < boxBack && pos[i].z > boxBack - boxWallWidth &&
        pos[i].y < boxTop) {
      v = (pos[i] - ppos[i]) / dt;
      norm = glm::vec3(0, 0, 1);
      vn = glm::dot(v, norm) * norm;
      vt = v - vn;
      I = vn - mu * vt;
      pos[i] += dt * I;
      bubbleGeneration(vel[i]);
    }
  }

  // Fix persisting collisions
  for (int i = 0; i < numParticles; i++) {
    pos[i].y = std::fmax(0, pos[i].y);

    if (pos[i].x < boxLeft && pos[i].x > boxLeft - boxWallWidth &&
        pos[i].y < boxTop) {
      pos[i].x = boxLeft;
    }
    if (pos[i].x > boxRight && pos[i].x < boxRight + boxWallWidth &&
        pos[i].y < boxTop) {
      pos[i].x = boxRight;
    }
    if (pos[i].z > boxFront && pos[i].z < boxFront + boxWallWidth &&
        pos[i].y < boxTop) {
      pos[i].z = boxFront;
    }
    if (pos[i].z < boxBack && pos[i].z > boxBack - boxWallWidth &&
        pos[i].y < boxTop) {
      pos[i].z = boxBack;
    }
  }
}

void SPHFluid::transferHeat() {
  // Heat conduction equation is Q/t = kA(T1 - T2) / d
  // Q/t = heat transfered over time, k = thermal conductivity, A = area, T1, T2
  // = temps, d = thickness of barrier simplify to spring Q/t = k(T1 - T2) / d,
  // where k is tuning param and d is distance also need to convect particles
  // relative to surrounding heat
  float k = 0.003;
  float pull = .008;
  float damp = 0.005;
  float pheat[maxParticles];
  for (int i = 0; i < numParticles; i++) {
    pheat[i] = heat[i];
  }

  // calculate transfer of heat
  for (int i = 0; i < numParticles; i++) {
    // heat the corners of the box
    if (ppos[i].y < boxBottom + r * 2 &&
        (ppos[i].x < boxLeft + r * 3 || ppos[i].x > boxRight - r * 3)) {
      // stimulate motion, prevents deadlock
      pos[i].y += .002;
      // add heat
      heat[i] += .01;
    }

    getNeighbors(i, &n);

    // how many neighbors are in direct vicinity and "above"
    int aboveNeighbors = 0;

    for (int j = 0; j < n.size; j++) {
      if (glm::length(ppos[i] - ppos[n.n[j]]) <= 4.0 * r ||
          (ppos[i].y < r && glm::length(ppos[i] - ppos[n.n[j]]) < 10.0 * r)) {
        heat[i] -= k * (pheat[i] - pheat[n.n[j]]) / 2.0;
        heat[n.n[j]] += k * (pheat[i] - pheat[n.n[j]]) / 2.0;
        vel[i].y += pull * (pheat[i] - pheat[n.n[j]]);
        if ((pheat[i] - pheat[n.n[j]]) > 0) {
          pos[n.n[j]].x += (pos[n.n[j]].x - pos[i].x) * .005;
          pos[i].y +=
              .008 * (pheat[i] - pheat[n.n[j]]) * (pheat[i] - pheat[n.n[j]]);
          pos[n.n[j]].y += .0008 * (pheat[i] - pheat[n.n[j]]);
        }
        if (ppos[i].y < ppos[n.n[j]].y) {
          aboveNeighbors += 1;
        }
      }
    }
    // check to see if it is exposed to open air, but make sure its not on the
    // bottom layer
    if (aboveNeighbors < 2 && pos[i].y > r * 3) {
      if (heat[i] > 0) {
        heat[i] -= damp;
      }
    }
  }

  return;
}

glm::vec3 SPHFluid::triangleSphereCollisionPoint(int v1, int v2, int v3,
                                                 int i) {
  // http://graphics.cs.aueb.gr/graphics/docs/papers/particle.pdf

  float inf = std::numeric_limits<float>::infinity();

  glm::vec3 e1 = ss->pos[v1] - ss->pos[v2];
  glm::vec3 e2 = ss->pos[v3] - ss->pos[v2];
  glm::vec3 n = glm::normalize(glm::cross(e1, e2));
  float d = -glm::dot(n, ss->pos[v2]);

  // Cylinder/triangle intersection

  // If no cylinder/triangle intersection, sphereical-cap/triangle intersection
  // Step 1: Check if plane intersects sphere
  float dist = glm::dot(n, pos[i]) + d;
  if (fabs(dist) > r) {
    // No Collision
    return glm::vec3(inf, inf, inf);
  }

  glm::vec3 veli = (pos[i] - ppos[i]) / dt;

  // Step 2: Check if any vertices are inside the sphere
  glm::vec3 vToC = pos[i] - ss->pos[v1];
  if (glm::length(vToC) < r) {
    // Collision
    glm::vec3 q = glm::proj(vToC, veli);
#ifdef DEBUG
    printf("Step 2a\n");
    printf("q = %f %f %f\n", q.x, q.y, q.z);
#endif
    if (glm::dot(vToC, veli) < 0) {
      return pos[i] - (r - glm::length(q)) * glm::normalize(veli);
    } else {
      return pos[i] - (r + glm::length(q)) * glm::normalize(veli);
    }
  }
  vToC = pos[i] - ss->pos[v2];
  if (glm::length(vToC) < r) {
    // Collision
    glm::vec3 q = glm::proj(vToC, veli);
#ifdef DEBUG
    printf("Step 2b\n");
    printf("q = %f %f %f\n", q.x, q.y, q.z);
#endif
    if (glm::dot(vToC, veli) < 0) {
      return pos[i] - (r - glm::length(q)) * glm::normalize(veli);
    } else {
      return pos[i] - (r + glm::length(q)) * glm::normalize(veli);
    }
  }
  vToC = pos[i] - ss->pos[v3];
  if (glm::length(vToC) < r) {
    // Collision
    glm::vec3 q = glm::proj(vToC, pos[i] - ppos[i]);
#ifdef DEBUG
    printf("Step 2c\n");
    printf("q = %f %f %f\n", q.x, q.y, q.z);
#endif
    if (glm::dot(vToC, veli) < 0) {
      return pos[i] - (r - glm::length(q)) * glm::normalize(veli);
    } else {
      return pos[i] - (r + glm::length(q)) * glm::normalize(veli);
    }
  }

  // Step 3: Project sphere onto triangle's plane, if projected center lies in
  // triangle, there is a collision
  // http://blackpawn.com/texts/pointinpoly/
  glm::vec3 cProj = pos[i] - dist * n;
  glm::vec3 a = ss->pos[v3] - ss->pos[v1];
  glm::vec3 b = ss->pos[v2] - ss->pos[v1];
  glm::vec3 c = cProj - ss->pos[v1];
  float dotaa = glm::dot(a, a);
  float dotab = glm::dot(a, b);
  float dotac = glm::dot(a, c);
  float dotbb = glm::dot(b, b);
  float dotbc = glm::dot(b, c);
  float invDenom = 1 / (dotaa * dotbb - dotab * dotab);
  float u = (dotbb * dotac - dotab * dotbc) * invDenom;
  float v = (dotaa * dotbc - dotab * dotac) * invDenom;
  if ((u >= 0) && (v >= 0) && (u + v < 1)) {
#ifdef DEBUG
    printf("Step 3\n");
#endif
    // Collision
    glm::vec3 projToC = pos[i] - cProj;
    glm::vec3 q = glm::proj(projToC, veli);
    if (glm::dot(projToC, veli) < 0) {
      return pos[i] - (r - glm::length(q)) * glm::normalize(veli);
    } else {
      return pos[i] - (r + glm::length(q)) * glm::normalize(veli);
    }
  }

  // Step 4: Check if the sphere intersects with triangle edge
  glm::vec3 p1, p2, edge, q, qToC;
  float t;

  p1 = ss->pos[v1];
  p2 = ss->pos[v2];
  edge = p2 - p1;
  t = (glm::dot(n, pos[i]) - glm::dot(n, ss->pos[v1])) / glm::dot(n, n);
  q = p1 + t * edge;
  qToC = pos[i] - q;
  dist = glm::length(qToC);
  if (dist < r) {
#ifdef DEBUG
    printf("Step 4a\n");
#endif
    q = glm::proj(qToC, veli);
    if (glm::dot(qToC, veli) < 0) {
      return pos[i] - (r - glm::length(q)) * glm::normalize(veli);
    } else {
      return pos[i] - (r + glm::length(q)) * glm::normalize(veli);
    }
  }

  p1 = ss->pos[v2];
  p2 = ss->pos[v3];
  edge = p2 - p1;
  t = (glm::dot(n, pos[i]) - glm::dot(n, ss->pos[v1])) / glm::dot(n, n);
  q = p1 + t * edge;
  qToC = pos[i] - q;
  dist = glm::length(qToC);
  if (dist < r) {
#ifdef DEBUG
    printf("Step 4b\n");
#endif
    if (glm::dot(qToC, veli) < 0) {
      return pos[i] - (r - glm::length(q)) * glm::normalize(veli);
    } else {
      return pos[i] - (r + glm::length(q)) * glm::normalize(veli);
    }
  }

  p1 = ss->pos[v3];
  p2 = ss->pos[v1];
  edge = p2 - p1;
  t = (glm::dot(n, pos[i]) - glm::dot(n, ss->pos[v1])) / glm::dot(n, n);
  q = p1 + t * edge;
  qToC = pos[i] - q;
  dist = glm::length(qToC);
  if (dist < r) {
#ifdef DEBUG
    printf("Step 4c\n");
#endif
    if (glm::dot(qToC, veli) < 0) {
      return pos[i] - (r - glm::length(q)) * glm::normalize(veli);
    } else {
      return pos[i] - (r + glm::length(q)) * glm::normalize(veli);
    }
  }

  return glm::vec3(inf, inf, inf);
}

void SPHFluid::clothInteraction() {
  if (!ss) return;
  glm::vec3 e1, e2, n;
  int tl, tr, bl, br;

  for (int k = 0; k < numParticles; k++) {
    float inf = std::numeric_limits<float>::infinity();
    float minCollisionDist = inf;
    // Iterate over triangles
    for (int i = 0; i < ss->width; i++) {
      for (int j = 0; j < ss->height; j++) {
        int ij = i * (ss->height + 1) + j;

        tl = ij;                       // top left
        tr = ij + ss->height + 1;      // top right
        bl = ij + 1;                   // bottom left
        br = ij + ss->height + 1 + 1;  // bottom right

        glm::vec3 q;
        float dist;

        // Top triangle
        q = triangleSphereCollisionPoint(bl, tl, tr, k);
        dist = glm::length(ppos[k] - q);
        if (dist < minCollisionDist) {
          minCollisionDist = dist;
          pos[k] = q;
#ifdef DEBUG
          printf("Contact at %f %f %f\n", pos[k].x, pos[k].y, pos[k].z);
#endif
        }

        // Bottom triangle
        q = triangleSphereCollisionPoint(tr, br, bl, k);
        dist = glm::length(ppos[k] - q);
        if (dist < minCollisionDist) {
          minCollisionDist = dist;
          pos[k] = q;
#ifdef DEBUG
          printf("Contact at %f %f %f\n", pos[k].x, pos[k].y, pos[k].z);
#endif
        }
      }
    }
  }
}

void SPHFluid::getNeighbors(int i, NeighborSet *n, bool pairwise) {
  n->size = 0;

#ifdef GRID
  glm::ivec3 g = getGridPos(&pos[i]);
  clampGridPos(&g);

  // Iterate over surroung grid cells
  for (int z = g.z - 1; z <= g.z + 1; z++) {
    if (z < 0 || z > gridSize.z - 1) continue;
    for (int y = g.y - 1; y <= g.y + 1; y++) {
      if (y < 0 || y > gridSize.y - 1) continue;
      for (int x = g.x - 1; x <= g.x + 1; x++) {
        if (x < 0 || x > gridSize.x - 1) continue;

        // Iterate over particles found in this grid cell
        glm::ivec3 gridPos(x, y, z);
        int cellId = getCellId(&gridPos);
        int ii = cellStart[cellId];
        if (ii < 0) {
          continue;
        }
        for (; ii < numParticles && cellId == particleHash[ii].x; ii++) {
          int j = particleHash[ii].y;
          if ((pairwise && i > j) || i == j) continue;
          if (glm::length(pos[i] - pos[j]) < h) {
            n->n[n->size] = j;
            n->size++;
          }
        }
      }
    }
  }
#else
  // Brute-force
  for (int j = 0; j < numParticles; j++) {
    if ((pairwise && i > j) || i == j) continue;
    if (glm::length(pos[i] - pos[j]) < h) {
      n->n[n->size] = j;
      n->size++;
    }
  }
#endif
}

glm::ivec3 SPHFluid::getGridPos(glm::vec3 *p) {
  glm::ivec3 gridPos;
  gridPos.x = floor((p->x - worldOrigin.x) / h);
  gridPos.y = floor((p->y - worldOrigin.y) / h);
  gridPos.z = floor((p->z - worldOrigin.z) / h);
  return gridPos;
}

void SPHFluid::clampGridPos(glm::ivec3 *gridPos) {
  gridPos->x = std::max(0, std::min(gridPos->x, gridSize.x - 1));
  gridPos->y = std::max(0, std::min(gridPos->y, gridSize.y - 1));
  gridPos->z = std::max(0, std::min(gridPos->z, gridSize.z - 1));
}

int SPHFluid::getCellId(glm::ivec3 *gridPos) {
  clampGridPos(gridPos);
  int cell = gridPos->z * gridSize.y * gridSize.x + gridPos->y * gridSize.x +
             gridPos->x;
  return cell;
}

void SPHFluid::findCellStart() {
  for (int i = 0; i < numParticles; i++) {
    int cell = particleHash[i].x;
    if (i > 0) {
      if (cell != particleHash[i - 1].x) cellStart[cell] = i;
    } else {
      cellStart[cell] = i;
    }
  }
}

bool ivec2_ascending(glm::ivec2 i, glm::ivec2 j) { return i.x < j.x; }

void SPHFluid::makeGrid() {
#ifdef GRID
  delete[] particleHash;
  delete[] cellStart;
  particleHash = new glm::ivec2[numParticles];
  cellStart = new int[maxCellId];
  for (int i = 0; i < maxCellId; i++) {
    cellStart[i] = -1;
  }

  for (int i = 0; i < numParticles; i++) {
    glm::ivec3 gridPos = getGridPos(&pos[i]);
    int cellId = getCellId(&gridPos);
    particleHash[i] = glm::ivec2(cellId, i);
  }

  std::sort(particleHash, particleHash + numParticles, ivec2_ascending);

  findCellStart();
#endif
}

glm::vec3 SPHFluid::initialParticlePosition(int i) {
  float x = 1.2 + 0.2 * rand01();
  float y = 1.2 + 0.2 * rand01();
  float z = 0.1 * rand01();
  return glm::vec3(x, y, z);
}

glm::vec3 SPHFluid::initialParticleVelocity(int i) {
  float x = -1.5 - 0.5 * rand01();
  float y = 2 + 1.5 * rand01();
  float z = 0.1 * rand01();
  return glm::vec3(x, y, z);
}

float SPHFluid::initialParticleHeat(int i) {
  if (rand01() > .5) {
    return 1.0;
  }
  return 0.01;
}

void SPHFluid::initVBO() {
  // Box vertices

  // 0
  vboData[0] = boxLeft - r;
  vboData[1] = boxBottom - r;
  vboData[2] = boxFront + r;

  // 1
  vboData[4] = boxRight + r;
  vboData[5] = boxBottom - r;
  vboData[6] = boxFront + r;

  // 1
  vboData[8] = boxRight + r;
  vboData[9] = boxBottom - r;
  vboData[10] = boxFront + r;

  // 2
  vboData[12] = boxRight + r;
  vboData[13] = boxTop + r;
  vboData[14] = boxFront + r;

  // 2
  vboData[16] = boxRight + r;
  vboData[17] = boxTop + r;
  vboData[18] = boxFront + r;

  // 3
  vboData[20] = boxLeft - r;
  vboData[21] = boxTop + r;
  vboData[22] = boxFront + r;

  // 3
  vboData[24] = boxLeft - r;
  vboData[25] = boxTop + r;
  vboData[26] = boxFront + r;

  // 0
  vboData[28] = boxLeft - r;
  vboData[29] = boxBottom - r;
  vboData[30] = boxFront + r;

  // 4
  vboData[32] = boxLeft - r;
  vboData[33] = boxBottom - r;
  vboData[34] = boxBack - r;

  // 5
  vboData[36] = boxRight + r;
  vboData[37] = boxBottom - r;
  vboData[38] = boxBack - r;

  // 5
  vboData[40] = boxRight + r;
  vboData[41] = boxBottom - r;
  vboData[42] = boxBack - r;

  // 6
  vboData[44] = boxRight + r;
  vboData[45] = boxTop + r;
  vboData[46] = boxBack - r;

  // 6
  vboData[48] = boxRight + r;
  vboData[49] = boxTop + r;
  vboData[50] = boxBack - r;

  // 7
  vboData[52] = boxLeft - r;
  vboData[53] = boxTop + r;
  vboData[54] = boxBack - r;

  // 7
  vboData[56] = boxLeft - r;
  vboData[57] = boxTop + r;
  vboData[58] = boxBack - r;

  // 4
  vboData[60] = boxLeft - r;
  vboData[61] = boxBottom - r;
  vboData[62] = boxBack - r;

  // 0
  vboData[64] = boxLeft - r;
  vboData[65] = boxBottom - r;
  vboData[66] = boxFront + r;

  // 4
  vboData[68] = boxLeft - r;
  vboData[69] = boxBottom - r;
  vboData[70] = boxBack - r;

  // 1
  vboData[72] = boxRight + r;
  vboData[73] = boxBottom - r;
  vboData[74] = boxFront + r;

  // 5
  vboData[76] = boxRight + r;
  vboData[77] = boxBottom - r;
  vboData[78] = boxBack - r;

  // 2
  vboData[80] = boxRight + r;
  vboData[81] = boxTop + r;
  vboData[82] = boxFront + r;

  // 6
  vboData[84] = boxRight + r;
  vboData[85] = boxTop + r;
  vboData[86] = boxBack - r;

  // 3
  vboData[88] = boxLeft - r;
  vboData[89] = boxTop + r;
  vboData[90] = boxFront + r;

  // 7
  vboData[92] = boxLeft - r;
  vboData[93] = boxTop + r;
  vboData[94] = boxBack - r;
}

void SPHFluid::updateVBO() {
  for (int i = 0; i < numParticles; i++) {
    vboData[4 * BOX_VERTICES + 4 * i + 0] = pos[i].x;
    vboData[4 * BOX_VERTICES + 4 * i + 1] = pos[i].y;
    vboData[4 * BOX_VERTICES + 4 * i + 2] = pos[i].z;
    vboData[4 * BOX_VERTICES + 4 * i + 3] = heat[i];
  }
}

int SPHFluid::vboSize() {
  return (numParticles + BOX_VERTICES) * 4 * sizeof(float);
}
