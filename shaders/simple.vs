#version 330

precision highp float;

in vec2 pos;
in vec2 tex_pos;

out vec2 out_tpos;

uniform vec2 resolution;

void main() {
  vec2 norm = pos / resolution;
  norm *= vec2(2.0, -2.0);
  norm += vec2(-1.0, 1.0);

  out_tpos = tex_pos;

  gl_Position = vec4(norm, 0.0, 1.0);
}
