#include "camera.h"

#define GLM_FORCE_RADIANS
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

Camera::Camera(glm::vec3 pos_, glm::vec3 target_, glm::vec3 worldUp_)
    : pos(pos_),
      worldUp(worldUp_),
      pitch(0),
      // yaw(-3.14159 / 2.),
      yaw(7 * 3.14159 / 4),
      fov(3.14159 / 6.),
      target(target_),
      moveSpeed(0.05),
      lookSpeed(0.5),
      mouseLook(SDL_FALSE),
      mouseSensitivity(1 / 500.) {
  front = glm::normalize(target - pos);
  direction = -front;
  right = glm::normalize(glm::cross(worldUp, direction));
  up = glm::cross(direction, right);
}

void Camera::processMouseWheel(SDL_Event* event) {
  if (event->wheel.y == 1) {
    fov *= 0.8;
  } else if (event->wheel.y == -1) {
    fov *= 1.25;
  }
}

void Camera::processMovement(const Uint8* keyState, float delta) {
  if (mouseLook) {
    int mx_, my_;
    SDL_GetRelativeMouseState(&mx_, &my_);
    float mx = mx_ * mouseSensitivity;
    float my = my_ * mouseSensitivity;
    yaw -= mx;
    pitch -= my;
  } else {
    if (keyState[SDL_SCANCODE_LEFT]) {
      yaw -= lookSpeed * delta;
    }
    if (keyState[SDL_SCANCODE_RIGHT]) {
      yaw += lookSpeed * delta;
    }
    if (keyState[SDL_SCANCODE_UP]) {
      pitch += lookSpeed * delta;
    }
    if (keyState[SDL_SCANCODE_DOWN]) {
      pitch -= lookSpeed * delta;
    }
  }

  float x = glm::half_pi<float>() - 0.001;
  pitch = std::min(std::max(pitch, -x), x);

  glm::vec3 mewFront;
  mewFront.x = std::cos(pitch) * std::cos(yaw);
  mewFront.y = std::sin(pitch);
  mewFront.z = std::cos(pitch) * std::sin(yaw);
  front = glm::normalize(mewFront);
  right = glm::normalize(glm::cross(front, worldUp));
  up = glm::normalize(glm::cross(right, front));

  moveSpeed = 2.5 * delta;

  // WASD
  if (keyState[SDL_SCANCODE_W]) {
    pos += moveSpeed * front;
  }
  if (keyState[SDL_SCANCODE_S]) {
    pos -= moveSpeed * front;
  }
  if (keyState[SDL_SCANCODE_D]) {
    pos += glm::normalize(glm::cross(front, up)) * moveSpeed;
  }
  if (keyState[SDL_SCANCODE_A]) {
    pos -= glm::normalize(glm::cross(front, up)) * moveSpeed;
  }
}

glm::mat4 Camera::getLookAt() { return glm::lookAt(pos, pos + front, up); }
