#version 450

layout(location = 0) out vec3 fragColor;
layout(push_constant) uniform PushConstants {
  vec2 offset;
  vec2 halfSize;
}
pc;

vec2 positions[6] = vec2[](vec2(-1.0, -1.0), vec2(1.0, -1.0), vec2(1.0, 1.0),
                           vec2(-1.0, -1.0), vec2(1.0, 1.0), vec2(-1.0, 1.0));

void main() {
  vec2 pos = positions[gl_VertexIndex] * pc.halfSize + pc.offset;
  gl_Position = vec4(pos, 0.0, 1.0);
  fragColor = vec3(1.0, 1.0, 1.0);
}
