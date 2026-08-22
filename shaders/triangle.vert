#version 450

layout(location = 0) out vec3 fragColor;
layout(push_constant) uniform PushConstants { vec2 offset; }
pc;

vec2 positions[6] = vec2[](vec2(-0.2, -0.3), vec2(0.2, -0.3), vec2(0.2, 0.3),
                           vec2(-0.2, -0.3), vec2(0.2, 0.3), vec2(-0.2, 0.3));

void main() {
  gl_Position = vec4(positions[gl_VertexIndex] + pc.offset, 0.0, 1.0);
  fragColor = vec3(1.0, 1.0, 1.0);
}
