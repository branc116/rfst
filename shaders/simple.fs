#version 330

precision highp float;

in vec2 out_tpos;
out vec4 out_color;

uniform sampler2D atlas;
uniform vec3 sub_pix_aa_map;
uniform float sub_pix_aa_scale;

void main() {
  vec4 v0 = texture(atlas, out_tpos);
  float sum = v0.x;
  float dx = dFdx(sum);
  vec3 c = sub_pix_aa_map * dx * sub_pix_aa_scale + sum;
  c = normalize(c) * sum;

  out_color = vec4(c, length(c));
}
