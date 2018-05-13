#version 150 core

in vec3 Color;
in vec3 pos;

out vec4 outColor;

const float ambient = .3;
void main() {
  outColor = vec4(Color, 1.0);
}
