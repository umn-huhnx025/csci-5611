#include "spring_system.h"

#include <algorithm>
#include <cstdio>

#include "sph_fluid.h"

const float SpringSystem::DRAG_COEF = -80;

SpringSystem::SpringSystem(int w, int h)
    : width(w),
      height(h),
      sphereR(0),
      spherePos(glm::vec3(0, 0, 0.5)),
      simSteps(100),
      k(20000),
      kv(2000),
      restLen(0.05),
      sphereSpeed(0.025),
      wind(glm::vec3(0, 0, 0)),
      gravity(0, -5, 0) {
  numNodes = (width + 1) * (height + 1);

  initBuffers();
  setInitialPositions();
  setTextureCoords();
  updateVertices();
}

SpringSystem::~SpringSystem() {
  delete[] pos;
  delete[] vel1;
  delete[] vel2;
  delete[] vertices;
  delete[] tex;
  delete[] norm;
  delete[] drag;
}

void SpringSystem::update(float dt) {
  dt = dt / (float)simSteps;
  float dt2 = dt / 2.;
  int ij1, ij2;
  glm::vec3 fv;
  for (int s = 0; s < simSteps; s++) {
    // Copy old velocities to new array
    std::copy(vel1, vel1 + numNodes, vel2);
    std::copy(vel1, vel1 + numNodes, vmid);

    // Horizontal spring forces
    for (int i = 0; i < width; i++) {
      for (int j = 0; j < height + 1; j++) {
        ij1 = i * (height + 1) + j;
        ij2 = ij1 + height + 1;

        fv = springForce(ij1, ij2, dt);
        vel2[ij1] += fv;
        vel2[ij2] -= fv;

        vmid[ij1] += fv / 2.f;
        vmid[ij2] -= fv / 2.f;
      }
    }

    // Vertical spring forces
    for (int i = 0; i < width + 1; i++) {
      for (int j = 0; j < height; j++) {
        ij1 = i * (height + 1) + j;
        ij2 = ij1 + 1;

        fv = springForce(ij1, ij2, dt);
        vel2[ij1] += fv;
        vel2[ij2] -= fv;

        vmid[ij1] += fv / 2.f;
        vmid[ij2] -= fv / 2.f;
      }
    }

    updateDrag();

    // Add gravity and drag
    glm::vec3 g = gravity * dt;
    for (int i = 0; i < numNodes; i++) {
      vel2[i] += g + drag[i] * dt;
      drag[i] = glm::vec3();
    }

    detectCollisions();

    fixNodes();

    for (int i = 0; i < numNodes; i++) {
      // Eulerian
      // pos[i] += vel2[i] * dt;

      // Midpoint
      pos[i] += vmid[i] * dt;
    }

    // Swap velocity buffers
    std::swap(vel1, vel2);
  }

  updateVertices();
}

void SpringSystem::initBuffers() {
  // Keep pointers to old and new arrays, saves one copy per simulation step
  vel1 = new glm::vec3[numNodes];
  vel2 = new glm::vec3[numNodes];
  vmid = new glm::vec3[numNodes];

  tex = new glm::vec2[numNodes];
  norm = new glm::vec3[numNodes];

  // Two nodes per vertex, each with 3 position coords, 3 normal components, and
  // 2 texture coords
  numVertices = 2 * (3 + 3 + 2) * width * (height + 1);
  vertices = new float[numVertices];

  drag = new glm::vec3[numVertices];
}

void SpringSystem::setInitialPositions() {
  float totalWidth = restLen * width;
  pos = new glm::vec3[numNodes];
  for (int i = 0; i < width + 1; i++) {
    pos[i * (height + 1)] =
        glm::vec3(-0.5 * totalWidth + i * restLen, 0.5, -0.25);
    for (int j = 1; j < height + 1; j++) {
      int ij = i * (height + 1) + j;
      pos[ij] = pos[ij - 1] - glm::vec3(0, 0, -restLen);
    }
  }
}

void SpringSystem::setTextureCoords() {
  for (int i = 0; i < numNodes; i++) {
    int u = i / (height + 1);
    int v = i % (height + 1);
    tex[i] = glm::vec2(u / (float)width, v / (float)height);
  }
}

glm::vec3 SpringSystem::springForce(int ij1, int ij2, float dt) {
  float l, v1, v2, f;
  glm::vec3 e, fv;
  e = pos[ij2] - pos[ij1];
  l = glm::length(e);
  e = glm::normalize(e);
  v1 = glm::dot(e, vel1[ij1]);
  v2 = glm::dot(e, vel1[ij2]);
  f = -k * (restLen - l) - kv * (v1 - v2);
  fv = f * dt * e;
  return fv;
}

void SpringSystem::fixNodes() {
  // Fix top and bottom rows
  for (int i = 0; i < width + 1; i++) {
    vel2[i * (height + 1)] = glm::vec3();
    vmid[i * (height + 1)] = glm::vec3();
    vel2[i * (height + 1) + height] = glm::vec3();
    vmid[i * (height + 1) + height] = glm::vec3();
  }
}

void SpringSystem::detectCollisions() {
  float d;
  glm::vec3 n, bounce;
  for (int i = 0; i < numNodes; i++) {
    d = glm::length(spherePos - pos[i]);
    if (d < sphereR + 0.009) {
      n = glm::normalize(-1.f * (spherePos - pos[i]));
      pos[i] += (0.01f + sphereR - d) * n;

      bounce = glm::dot(vel2[i], n) * n;
      vel2[i] -= 1.5f * bounce;

      bounce = glm::dot(vmid[i], n) * n;
      vmid[i] -= 1.5f * bounce;
    }
  }
}

void SpringSystem::updateDrag() {
  int tl, tr, bl, br;
  glm::vec3 e1, e2, v, n, dragF;
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      int ij = i * (height + 1) + j;

      tl = ij;                   // top left
      tr = ij + height + 1;      // top right
      bl = ij + 1;               // bottom left
      br = ij + height + 1 + 1;  // bottom right

      // Top triangle
      v = (vel1[tl] + vel1[tr] + vel1[bl]) / 3.f - wind;
      e1 = pos[bl] - pos[tl];
      e2 = pos[tr] - pos[tl];
      n = glm::cross(e1, e2);
      dragF = (float)(DRAG_COEF * glm::length(v) * glm::dot(v, n)) *
              glm::normalize(n);
      drag[tl] += dragF / 3.f;
      drag[tr] += dragF / 3.f;
      drag[bl] += dragF / 3.f;

      // Bottom triangle
      v = (vel1[bl] + vel1[br] + vel1[tr]) / 3.f - wind;
      e1 = pos[tr] - pos[br];
      e2 = pos[bl] - pos[br];
      n = glm::cross(e1, e2);
      dragF = (float)(DRAG_COEF * glm::length(v) * glm::dot(v, n)) *
              glm::normalize(n);
      drag[tr] += dragF / 3.f;
      drag[br] += dragF / 3.f;
      drag[bl] += dragF / 3.f;
    }
  }
}

void SpringSystem::updateVertices() {
  // Update vertex buffer data
  updateNormals();
  int pij, vij;
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height + 1; j++) {
      vij = i * (height + 1) + j;

      // Left node position
      pij = vij;
      vertices[16 * vij + 0] = pos[pij].x;
      vertices[16 * vij + 1] = pos[pij].y;
      vertices[16 * vij + 2] = pos[pij].z;

      // Normal
      vertices[16 * vij + 3] = norm[pij].x;
      vertices[16 * vij + 4] = norm[pij].y;
      vertices[16 * vij + 5] = norm[pij].z;

      // Texture coords
      vertices[16 * vij + 6] = tex[pij].x;
      vertices[16 * vij + 7] = tex[pij].y;

      // Right node position
      pij = vij + height + 1;
      vertices[16 * vij + 8] = pos[pij].x;
      vertices[16 * vij + 9] = pos[pij].y;
      vertices[16 * vij + 10] = pos[pij].z;

      // Normal
      vertices[16 * vij + 11] = norm[pij].x;
      vertices[16 * vij + 12] = norm[pij].y;
      vertices[16 * vij + 13] = norm[pij].z;

      // Texture coords
      vertices[16 * vij + 14] = tex[pij].x;
      vertices[16 * vij + 15] = tex[pij].y;
    }
  }
}

void SpringSystem::updateNormals() {
  // Zero out normals
  for (int i = 0; i < numNodes; i++) {
    norm[i] = glm::vec3();
  }

  // Compute new normals
  glm::vec3 e1, e2, n;
  int tl, tr, bl, br;
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      int ij = i * (height + 1) + j;

      tl = ij;                   // top left
      tr = ij + height + 1;      // top right
      bl = ij + 1;               // bottom left
      br = ij + height + 1 + 1;  // bottom right

      // Top triangle
      e1 = pos[bl] - pos[tl];
      e2 = pos[tr] - pos[tl];
      n = glm::cross(e1, e2);
      norm[tl] += n;
      norm[tr] += n;
      norm[bl] += n;

      // Bottom triangle
      e1 = pos[tr] - pos[br];
      e2 = pos[bl] - pos[br];
      n = glm::cross(e1, e2);
      norm[bl] += n;
      norm[br] += n;
      norm[tr] += n;
    }
  }
}

void SpringSystem::moveBall(const Uint8* keyState) {
  if (keyState[SDL_SCANCODE_I]) {
    spherePos += sphereSpeed * glm::vec3(0, 0, 1);
  }
  if (keyState[SDL_SCANCODE_K]) {
    spherePos -= sphereSpeed * glm::vec3(0, 0, 1);
  }
  if (keyState[SDL_SCANCODE_J]) {
    spherePos -= sphereSpeed * glm::vec3(1, 0, 0);
  }
  if (keyState[SDL_SCANCODE_L]) {
    spherePos += sphereSpeed * glm::vec3(1, 0, 0);
  }
}
