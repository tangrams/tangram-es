#ifdef GL_ES
precision mediump float;
#endif

uniform float u_time;

varying vec4 v_color;

void main(void) {
  gl_FragColor = v_color;
}
