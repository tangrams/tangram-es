#ifdef GL_ES
precision highp float;
#endif

uniform mat4 u_modelViewProj;

attribute vec4 a_position;

void main() {
    gl_Position = u_modelViewProj * a_position;
}
