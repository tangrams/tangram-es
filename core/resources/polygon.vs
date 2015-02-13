#ifdef GL_ES
precision highp float;
#endif

uniform mat4 u_modelView;
uniform mat4 u_modelViewProj;
uniform mat3 u_normalMatrix;
uniform float u_time;
uniform float u_tileDepthOffset;

#pragma tangram: material
#pragma tangram: vertex_lighting

attribute vec4 a_position;
attribute vec4 a_color;
attribute vec3 a_normal;
attribute vec2 a_texcoord;

varying vec4 v_color;
varying vec3 v_eyeToPoint;
varying vec3 v_normal;
varying vec2 v_texcoord;

void main() {

    v_normal = normalize(a_normal);
    v_texcoord = a_texcoord;

    v_eyeToPoint = vec3(u_modelView * a_position);

    v_color = a_color;

    #ifdef TANGRAM_LIGHTS
        v_normal = normalize(u_normalMatrix * v_normal);
        lightVertex(v_eyeToPoint, v_normal, v_color);
    #endif

    gl_Position = u_modelViewProj * a_position;
    
    gl_Position.z /= u_tileDepthOffset;
}
