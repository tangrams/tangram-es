#ifdef GL_ES
precision mediump float;
#endif
uniform mat4 u_modelViewProj;
uniform vec4 u_lightDirection;
attribute vec4 a_position;
attribute vec4 a_normal;
attribute vec4 a_color;
varying vec4 v_color;
void main() {
  float lit = dot(normalize(u_lightDirection), normalize(a_normal));
  v_color = a_color;
  v_color.rgb *= clamp(lit * 1.5, 0.5, 1.5);
  gl_Position = u_modelViewProj * a_position;
}
