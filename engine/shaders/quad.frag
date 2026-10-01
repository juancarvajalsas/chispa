#version 450

layout(location = 0) in vec2 fragUV;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D tex;

layout(push_constant) uniform PushConstants {
  vec2 offset;
  vec2 halfSize;
  mat4 proj;
  vec2 uvOffset;
  vec2 uvScale;
  int useTexture;
  float angle;
}
pc;
void main() {
  if (pc.useTexture == 1) {
    outColor = texture(tex, fragUV);
  } else {
    outColor = vec4(1.0, 1.0, 1.0, 1.0);
  }
}
