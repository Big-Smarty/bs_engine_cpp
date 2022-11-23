#version 460


void main() {
  const vec3 vertices[3] = vec3[3] (
    vec3(1.f, 1.f, 0.f),
    vec3(-1.f, 1.f, 0.f),
    vec3(0.f, -1.f, 0.f)
  );

  gl_Position = vec4(vertices[gl_VertexIndex], 1.f);
}
