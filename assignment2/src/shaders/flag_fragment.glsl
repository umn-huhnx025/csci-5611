#version 150 core

in vec3 Color;
in vec3 normal;
in vec3 pos;
in vec3 light1Dir;
in vec3 light1Pos;
in vec3 light2Dir;
in vec3 light2Pos;
in vec2 texcoord;

out vec4 outColor;

uniform sampler2D tex0;
uniform sampler2D tex1;

uniform int texID;


const float ambient = .3;
void main() {
  vec3 color;
  if (texID == -1)
    color = Color;
  else if (texID == 0)
    color = texture(tex0, texcoord).rgb;
  else if (texID == 1)
    color = texture(tex1, texcoord).rgb;
  else {
    outColor = vec4(1,0,0,1);
    return; //This was an error, stop lighting!
  }
  vec3 diffuseC = color*max(dot(light1Dir,normal),0.0) / pow(length(light1Pos-pos), 4) * vec3(1,1,1)*20;
  diffuseC += color*max(dot(light2Dir,normal),0.0) / pow(length(light2Pos-pos), 4) * vec3(1,1,1)*20;
  vec3 ambC = color*ambient;
  vec3 oColor = ambC+diffuseC;
  outColor = vec4(oColor,1);
}
