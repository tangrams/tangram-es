#ifdef GL_ES
precision mediump float;
#endif

varying vec4 v_pos;
varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_texcoord;

void main(void) {
  	gl_FragColor = v_color;
}
