#pragma once

#include <SDL2/SDL.h>

#include "glm/glm.hpp"

class Camera {
 public:
  glm::vec3 pos;
  glm::vec3 front;
  glm::vec3 up;
  glm::vec3 worldUp;
  float pitch;
  float yaw;
  float fov;

  Camera(glm::vec3 pos_, glm::vec3 target_, glm::vec3 worldUp_);

  void processMouseWheel(SDL_Event* event);
  void processMovement(const Uint8* keyState, float delta);

  glm::mat4 getLookAt();

 private:
  glm::vec3 target;
  glm::vec3 direction;
  glm::vec3 right;
  float moveSpeed;
  float lookSpeed;

  SDL_bool mouseLook;
  float mouseSensitivity;
};
