#pragma once

#include "spring_system.h"

class Flag : public SpringSystem {
 public:
  Flag();

 private:
  void setInitialPositions();
  void setTextureCoords();
  void fixNodes();
  void detectCollisions();
};
