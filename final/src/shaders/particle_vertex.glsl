#version 140

in vec3 position;
in float heat;

const vec3 inLightDir = normalize(vec3(1,-1,1));

out vec3 Color;
out vec3 lightDir;
out vec4 eyePos;
out mat4 projMat;

uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
uniform vec3 inColor;
uniform bool useHeat;

const float pointScale = 70;
void main() {
  if (useHeat)
    Color = vec3(heat, 0, 1-heat);
  else
    Color = vec3(0.05, 0.35, 1);

  eyePos = view*model*vec4(position.xyz, 1.0);
  gl_PointSize = -pointScale/eyePos.z;
  gl_Position = proj * eyePos;
  lightDir = inLightDir;
  projMat = proj;
}
