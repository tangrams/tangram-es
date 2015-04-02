#ifdef GL_ES
precision highp float;
#endif

uniform mat4 u_modelView;
uniform mat4 u_modelViewProj;
uniform float u_time;

varying vec4 v_pos;
varying vec4 v_color;
varying vec3 v_eyeToPoint;
varying vec3 v_normal;
varying vec2 v_texcoord;

void main(void) {
    
    gl_FragColor = v_color;
    
}
