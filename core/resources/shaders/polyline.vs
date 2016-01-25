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
attribute vec4 a_extrude;
attribute vec2 a_texcoord;

varying vec4 v_world_position;
varying vec4 v_position;
varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_texcoord;

#ifdef TANGRAM_LIGHTING_VERTEX
    varying vec4 v_lighting;
#endif

#define UNPACK_POSITION(x) (x / 1024.0)
#define UNPACK_EXTRUSION(x) (x / 4096.0)
#define UNPACK_ORDER(x) (x / 2.0)

vec4 modelPosition() {
    return vec4(UNPACK_POSITION(a_position.xyz), 1.0);
}

vec4 worldPosition() {
    return v_world_position;
}

vec3 worldNormal() {
    return vec3(0.0, 0.0, 1.0);
}

#pragma tangram: material
#pragma tangram: lighting
#pragma tangram: global

void main() {

    // Initialize globals
    #pragma tangram: setup

    vec4 position = vec4(UNPACK_POSITION(a_position.xyz), 1.0);

    v_color = a_color;
    v_texcoord = a_texcoord;
    v_normal = u_normalMatrix * vec3(0.,0.,1.);

    {
        vec4 extrude = UNPACK_EXTRUSION(a_extrude);
        float width = extrude.z;
        float dwdz = extrude.w;
        float dz = u_map_position.z - abs(u_tile_origin.z);
        // Interpolate between zoom levels
        width += dwdz * clamp(dz, 0.0, 1.0);
        // Scale pixel dimensions to be consistent in screen space
        width *= exp2(-dz);

        // Modify line width in model space before extrusion
        #pragma tangram: width

        position.xy += extrude.xy * width;
    }

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

    // Proxy tiles have u_tile_origin.z < 0, so this adjustment will place proxy tiles 
    // deeper in the depth buffer than non-proxy tiles
    gl_Position.z += TANGRAM_DEPTH_DELTA * gl_Position.w * (1. - sign(u_tile_origin.z));

    #ifdef TANGRAM_DEPTH_DELTA
        float layer = UNPACK_ORDER(a_position.w);
        gl_Position.z -= layer * TANGRAM_DEPTH_DELTA * gl_Position.w;
    #endif
}
