#ifdef GL_ES
precision mediump float;
#define LOWP lowp
#else
#define LOWP
#endif

attribute vec2 a_position;
attribute vec2 a_uv;

varying vec2 v_uv;

uniform mat4 u_proj;

void main(void) {
    gl_Position = u_proj * vec4(a_position, 0.0, 1.0);
    v_uv = a_uv;
}
