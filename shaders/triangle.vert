#version 450

layout(location = 0) out vec2 fragUV;
layout(push_constant) uniform PushConstants {
  vec2 offset;
  vec2 halfSize;
  mat4 proj;
}
pc;

vec2 positions[6] = vec2[](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
                           vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));

vec2 uvs[6] = vec2[](vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(1.0, 1.0),
                     vec2(0.0, 0.0), vec2(1.0, 1.0), vec2(0.0, 1.0));

void main() {
  vec2 worldPos = positions[gl_VertexIndex] * pc.halfSize + pc.offset;
  gl_Position = pc.proj * vec4(worldPos, 0.0, 1.0);
  fragUV = uvs[gl_VertexIndex];
}
