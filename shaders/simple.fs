#version 330

precision highp float;

in vec2 out_tpos;
out vec4 out_color;

uniform sampler2D atlas;

void main() {
  float out_aa = 0.00001;
  vec4 v0 = texture(atlas, out_tpos);
  float sum = v0.x;
  sum *= 0.9;

  out_color = vec4(sum, sum, sum, length(v0.xyz));
}
