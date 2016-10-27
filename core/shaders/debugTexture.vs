#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 u_proj;

attribute vec2 a_position;
attribute vec2 a_uv;

varying vec2 uv;

void main() {
    uv = a_uv;
    gl_Position = u_proj * vec4(a_position, 1.0, 1.0);
}

