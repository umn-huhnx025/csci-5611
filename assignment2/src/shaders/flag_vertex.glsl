#version 150 core

in vec3 position;

const vec3 inLight1Pos = vec3(0,-2,-2.5);
const vec3 inLight2Pos = vec3(2,-2.2,-0.5);
//const vec3 inLightDir = normalize(vec3(0,-1,-1));
in vec3 inNormal;
in vec2 inTexcoord;

out vec3 Color;
out vec3 normal;
out vec3 pos;
out vec3 light1Pos;
out vec3 light1Dir;
out vec3 light2Pos;
out vec3 light2Dir;
out vec2 texcoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform vec3 inColor;

void main() {
  Color = inColor;
  gl_Position = proj * view * model * vec4(position,1.0);
  pos = (view * model * vec4(position,1.0)).xyz;
  light1Pos = (view * vec4(inLight1Pos,1.0)).xyz;
  light1Dir = normalize(light1Pos - pos);
  light2Pos = (view * vec4(inLight2Pos,1.0)).xyz;
  light2Dir = normalize(light2Pos - pos);
  vec4 norm4 = transpose(inverse(view*model)) * vec4(inNormal,0.0);
  normal = normalize(norm4.xyz);
  texcoord = inTexcoord;
}
