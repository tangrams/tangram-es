#pragma tangram: extensions

#ifdef GL_ES
precision highp float;
#endif

#pragma tangram: defines

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;
uniform mat3 u_normalMatrix;
uniform vec3 u_map_position;
uniform vec3 u_tile_origin;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_meters_per_pixel;
uniform float u_device_pixel_ratio;

#pragma tangram: uniforms

attribute vec4 a_position;
attribute vec4 a_color;
attribute vec3 a_normal;
attribute vec2 a_texcoord;
attribute float a_layer;

varying vec4 v_world_position;
varying vec4 v_position;
varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_texcoord;

#ifdef TANGRAM_LIGHTING_VERTEX
    varying vec4 v_lighting;
#endif

vec4 modelPosition() {
    return a_position;
}

vec4 worldPosition() {
    return v_world_position;
}

#pragma tangram: material
#pragma tangram: lighting
#pragma tangram: global

void main() {

    vec4 position = a_position;

    v_color = a_color;
    v_texcoord = a_texcoord;
    v_normal = normalize(u_normalMatrix * a_normal);

    // Transform position into meters relative to map center
    position = u_model * position;

    // World coordinates for 3d procedural textures
    vec4 local_origin = vec4(u_map_position.xy, 0., 0.);
    #ifdef TANGRAM_WORLD_POSITION_WRAP
        local_origin = mod(local_origin, TANGRAM_WORLD_POSITION_WRAP);
    #endif
    v_world_position = position + local_origin;

    // Modify position before lighting and camera projection
    #pragma tangram: position

    // Set position varying to the camera-space vertex position
    v_position = u_view * position;

    #ifdef TANGRAM_LIGHTING_VERTEX
        vec4 color = v_color;
        vec3 normal = v_normal;

        // Modify normal before lighting
        #pragma tangram: normal

        // Modify color and material properties before lighting
        #pragma tangram: color

        v_lighting = calculateLighting(v_position.xyz, normal, color);
        v_color = color;
        v_normal = normal;
    #endif

    gl_Position = u_proj * v_position;

    // Proxy tiles have u_tile_origin.z < 0, so this re-scaling will place proxy tiles deeper in
    // the depth buffer than non-proxy tiles by a distance that increases with tile zoom
    gl_Position.z /= 1. + .1 * (abs(u_tile_origin.z) - u_tile_origin.z);

    #ifdef TANGRAM_DEPTH_DELTA
        gl_Position.z -= a_layer * TANGRAM_DEPTH_DELTA * gl_Position.w;
    #endif
}
