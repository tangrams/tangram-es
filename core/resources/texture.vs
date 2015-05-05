#ifdef GL_ES
precision mediump float;
#define LOWP lowp
#else
#define LOWP
#endif

attribute vec3 a_position;
attribute vec2 a_uv;

varying vec2 v_uv;

void main(void) {
    gl_Position = vec4(a_position, 1.0);
    v_uv = a_uv;
}
