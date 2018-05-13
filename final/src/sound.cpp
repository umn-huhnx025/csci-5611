#include "sound.h"

#include <SDL.h>
#include <SDL_opengl.h>

#include "glm/glm.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <cstdint>
#include <stdlib.h>

short* audioBufferGeneration(bubbleSound audio, int bufferSize, float dt){
  short* sampleBuffer = new short[bufferSize];

  float radius = audio.radius;
  float audio_volume = 6000.0f;
  float resonant_frequency = 3.0f / radius;
  float damping = 0.13f / radius + 0.0072f * powf(radius, -1.5f);
  float e = 0.1f;

  short total = 0;
  for(int i = 0; i < bufferSize; i++){
    sampleBuffer[i] = audio_volume * sin(2 * 3.1416f * audio.audioPosition * resonant_frequency * (1.0f + e * damping * audio.audioPosition)) * powf(2.7f, -audio.audioPosition * damping);
    audio.audioPosition = audio.audioPosition +  dt / bufferSize;
  }

  return sampleBuffer;
}

bool siftBubbles(float audioPosition){
  for(int i = 0; i < bubbles.size(); i++){
    if(abs(audioPosition - bubbles[i].audioPosition) < 0.01f){
      return true;
    }
  }
  return false;
}

void bubbleGeneration(glm::vec3 velocity){
  if(bubbles.size() < 25){
    float r = (rand() % 286 + 15) / 10000.0f;
    float length = sqrtf(glm::dot(velocity, velocity));
    if(length > 0.7f){
      //printf("%f\n", r);
      bubbleSound b;
      b.radius = r  / length;
      if(b.radius > 0.03f){
        b.radius = 0.03f;
      }
      if(b.radius < 0.0015f){
        b.radius = 0.0015f;
      }
      b.audioPosition = rand() % 1999 / 2000.0f;
      //if(!siftBubbles(b.audioPosition)){
        bubbles.push_back(b);
      //}
    }
  }
}
