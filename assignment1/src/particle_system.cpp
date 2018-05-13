#include "particle_system.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>

// #include <glm/gtc/type_ptr.hpp>

// Library to load obj files
// https://github.com/syoyo/tinyobjloader
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

const glm::vec3 ParticleSystem::GRAVITY(0, -9.8, 0);

ParticleSystem::ParticleSystem(int numParticles_, const char* modelFile)
    : spawnRate(numParticles_ / 10.),
      spawnError(0.),
      modelScale(glm::vec3(1, 1, 1) * 0.015f),
      maxAge(3.),
      radius(0.01),
      maxParticles(numParticles_),
      numParticles(0),
      data(new float[3 * maxParticles]) {
  positions = new glm::vec3[maxParticles];
  colors = new glm::vec3[maxParticles];
  velocities = new glm::vec3[maxParticles];
  lifetimes = new float[maxParticles];
  spawnRate = maxParticles / maxAge;
  // transparencies = new float[num_particles];

  modelData = ParticleSystem::loadModel(modelFile);
}

ParticleSystem::~ParticleSystem() {
  delete positions;
  delete colors;
  delete velocities;
  delete lifetimes;
}

void ParticleSystem::update(float dt) {
  // printf("dt = %f\n", dt);
  // fflush(stdout);
  for (int i = 0; i < numParticles; i++) {
    // lifetimes[i] += dt;
    // if (lifetimes[i] > maxAge) {
    //   newParticle(i);
    // }
    velocities[i] += GRAVITY * dt;
    positions[i] += velocities[i] * dt;

    if (positions[i].z < radius) {
      positions[i].z = radius;
      velocities[i].z *= -1;
    }
  }
  spawnNewParticles(dt);
  updatePosData();
}

void ParticleSystem::newParticle(int i) {
  positions[i] = initialParticlePosition(i);
  velocities[i] = initialParticleVelocity(i);
  lifetimes[i] = 0.;
  colors[i] = initialParticleColor(i);
}

void ParticleSystem::spawnNewParticles(float dt) {
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

glm::vec3 ParticleSystem::initialParticlePosition(int i) {
  return glm::vec3(0, 0, 2);
}

glm::vec3 ParticleSystem::initialParticleVelocity(int i) {
  return glm::vec3(0, 0, 0);
}

glm::vec3 ParticleSystem::initialParticleColor(int i) {
  return glm::vec3(1., 1., 1.);
}

std::vector<tinyobj::real_t> ParticleSystem::loadModel(const char* filename) {
  std::string inputfile = filename;
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;

  std::string err;
  bool ret =
      tinyobj::LoadObj(&attrib, &shapes, &materials, &err, inputfile.c_str());
  if (!err.empty()) {  // `err` may contain warning message.
    std::cerr << err << std::endl;
  }

  if (!ret) {
    exit(1);
  }

  std::vector<tinyobj::real_t> model;

  // Loop over shapes
  for (size_t s = 0; s < shapes.size(); s++) {
    // Loop over faces(polygon)
    size_t index_offset = 0;
    for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
      unsigned fv = shapes[s].mesh.num_face_vertices[f];
      // Loop over vertices in the face.
      for (size_t v = 0; v < fv; v++) {
        tinyobj::real_t vx, vy, vz, nx, ny, nz, tx, ty;
        // access to vertex
        tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
        vx = attrib.vertices[3 * idx.vertex_index + 0];
        vy = attrib.vertices[3 * idx.vertex_index + 1];
        vz = attrib.vertices[3 * idx.vertex_index + 2];
        if (attrib.normals.size()) {
          nx = attrib.normals[3 * idx.normal_index + 0];
          ny = attrib.normals[3 * idx.normal_index + 1];
          nz = attrib.normals[3 * idx.normal_index + 2];
        } else {
          nx = 0.;
          ny = 0.;
          nz = 0.;
        }
        if (attrib.texcoords.size()) {
          tx = attrib.texcoords[2 * idx.texcoord_index + 0];
          ty = attrib.texcoords[2 * idx.texcoord_index + 1];
        } else {
          tx = 0.;
          ty = 0.;
        }
        tinyobj::real_t data[8] = {vx, vy, vz, tx, ty, nx, ny, nz};
        model.insert(model.end(), data, data + 8);
      }
      index_offset += fv;
    }
  }
  return model;
}

void ParticleSystem::updatePosData() {
  // memset(data, 0, 3 * maxParticles * sizeof(*data));
  for (int i = 0; i < numParticles; i++) {
    data[3 * i + 0] = positions[i].x;
    data[3 * i + 1] = positions[i].y;
    data[3 * i + 2] = positions[i].z;
  }

  // for (int i = 0; i < 3 * numParticles; i++) {
  //   printf("data[%d] = %f\n", i, data[i]);
  // }
  // printf("\n");
}
