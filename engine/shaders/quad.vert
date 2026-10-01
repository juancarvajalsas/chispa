#version 450

layout(location = 0) out vec2 fragUV;
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

vec2 positions[6] = vec2[](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
                           vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));

vec2 baseUV[6] = vec2[](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0),
                        vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0));

void main() {
  vec2 local = positions[gl_VertexIndex] * pc.halfSize;
  float c = cos(pc.angle);
  float s = sin(pc.angle);
  vec2 rotated = vec2(local.x * c - local.y * s, local.x * s + local.y * c);
  vec2 worldPos = rotated + pc.offset;
  gl_Position = pc.proj * vec4(worldPos, 0.0, 1.0);
  fragUV = pc.uvOffset + baseUV[gl_VertexIndex] * pc.uvScale;
}
