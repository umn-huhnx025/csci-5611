#version 140

in vec3 position;

out vec3 Color;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform vec3 inColor;

void main() {
  Color = inColor;
  gl_Position = proj * view * model * vec4(position,1.0);
}
