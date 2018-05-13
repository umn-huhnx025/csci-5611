#version 140
out vec4 oColor;

in vec3 Color;
in vec3 lightDir;
in vec4 eyePos;
in mat4 projMat;

const float pointScale = 80.0;
const float amb = 0.1;
const float far = -25;
const float near = .2;

void main(void) {
  oColor = vec4(Color, 1.0);
  vec3 N;
  N.xy = gl_PointCoord.st * 2.0 - vec2(1.0);
  float mag = dot(N.xy, N.xy);
  if (mag > 1.0) discard;   // kill pixels outside circle
  N.z = sqrt(1.0-mag);

  //calculate depth
  vec4 pixelPos = vec4(eyePos.xyz + N * pointScale, 1.0);
  vec4 clipSpacePos = projMat * pixelPos;
  float fragDepth = clipSpacePos.z / clipSpacePos.w;


  // calculate lighting
  float diffuse = max(0.0, dot(lightDir, N));


  oColor = vec4(Color,.5) * diffuse + vec4(amb, amb, amb, 0);
  
  
}
