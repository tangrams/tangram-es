#ifdef GL_ES
precision mediump float;
#endif

attribute vec2 a_position;

uniform mat4 u_proj;

void main() {
    gl_Position = u_proj * vec4(a_position, 1.0, 1.0);
}

