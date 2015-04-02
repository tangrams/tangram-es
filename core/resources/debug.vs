#ifdef GL_ES
precision highp float;
#endif

uniform mat4 u_modelViewProj;

attribute vec4 a_position;
attribute vec4 a_color;

varying vec4 v_color;

void main() {

    v_color = a_color;

    gl_Position = u_modelViewProj * a_position;
    gl_Position.z = 0.0; // Debug geometry draws above everything else
}
