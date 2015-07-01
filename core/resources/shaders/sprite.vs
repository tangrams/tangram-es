#ifdef GL_ES
precision mediump float;
#define LOWP lowp
#else
#define LOWP
#endif

attribute vec2 a_position;
attribute vec2 a_uv;
attribute float a_alpha;

varying vec2 v_uv;
varying float v_alpha;

uniform mat4 u_proj;

void main(void) {
    if (a_alpha != 0.0) {
        gl_Position = u_proj * vec4(a_position, 0.0, 1.0);
        v_uv = a_uv;
        v_alpha = a_alpha;
    } else {
        // clip this vertex
        gl_Position = vec4(0.0);
    }
}
