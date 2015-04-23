#ifdef GL_ES
precision highp float;
#endif

uniform mat4 u_modelView;
uniform mat4 u_modelViewProj;
uniform mat3 u_normalMatrix;
uniform float u_time;
uniform float u_zoom;
uniform float u_tile_zoom;

attribute vec4 a_position;
attribute vec4 a_color;

attribute vec3 a_extrudeNormal;
attribute float a_extrudeWidth;

attribute vec3 a_normal;
attribute vec2 a_texcoord;

attribute float a_layer;

varying vec4 v_color;
varying vec3 v_eyeToPoint;
varying vec3 v_normal;
varying vec2 v_texcoord;

#pragma tangram: material
#pragma tangram: lighting
#pragma tangram: globals

void main() {

    v_normal = vec3(0.,0.,1.);

    v_color = a_color;
    v_texcoord = a_texcoord;

    vec4 v_pos = a_position;
    v_pos.xyz += a_extrudeNormal * (a_extrudeWidth * 2.) * pow(2., abs(u_tile_zoom) - u_zoom);

    v_eyeToPoint = vec3(u_modelView * a_position);
    
    #ifdef TANGRAM_LIGHTING_VERTEX
        vec4 color = v_color;
        vec3 normal = normalize(u_normalMatrix * v_normal);

        // Modify normal before lighting
        #pragma tangram: normal

        // Modify color and material properties before lighting
        #pragma tangram: color

        v_lighting = calculateLighting(v_eyeToPoint.xyz, normal, color);
        v_color = color;
    #else
        v_normal = normalize(u_normalMatrix * v_normal);
        v_color = a_color;
    #endif

    gl_Position = u_modelViewProj * v_pos;
    
    // Proxy tiles have u_tile_zoom < 0, so this re-scaling will place proxy tiles deeper in
    // the depth buffer than non-proxy tiles by a distance that increases with tile zoom
    gl_Position.z /= 1. + .1 * (abs(u_tile_zoom) - u_tile_zoom);
    
    #ifdef TANGRAM_DEPTH_DELTA
        gl_Position.z -= a_layer * TANGRAM_DEPTH_DELTA * gl_Position.w;
    #endif
}
