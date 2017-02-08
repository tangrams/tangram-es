#pragma tangram: extensions

#ifdef GL_ES
precision highp float;
#endif

#pragma tangram: defines

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;
uniform mat3 u_normal_matrix;
uniform vec4 u_tile_origin;
uniform vec3 u_map_position;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_meters_per_pixel;
uniform float u_device_pixel_ratio;
uniform float u_proxy_depth;

#pragma tangram: uniforms

attribute vec4 a_position;
attribute vec4 a_color;

varying vec4 v_world_position;
varying vec4 v_position;
varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_texcoord;

#ifdef TANGRAM_LIGHTING_VERTEX
    varying vec4 v_lighting;
#endif

vec4 modelPosition() {
    return vec4(a_position.xyz * exp2(u_tile_origin.z - u_tile_origin.w), 1.0);
}

vec4 worldPosition() {
    return v_world_position;
}

vec3 worldNormal() {
    return vec3(0., 0., 1.);
}

vec4 modelPositionBaseZoom() {
    return vec4(a_position.xyz, 1.0);
}

#pragma tangram: material
#pragma tangram: lighting
#pragma tangram: global
#pragma tangram: raster

#ifdef TANGRAM_MODEL_POSITION_BASE_ZOOM_VARYING
    varying vec4 v_modelpos_base_zoom;
#endif

void main() {

    vec4 position = vec4(a_position.xy, 0.0, 1.0);

    // Initialize globals
    #pragma tangram: setup

    v_color = a_color;

    v_texcoord = a_position.xy;

    #ifdef TANGRAM_MODEL_POSITION_BASE_ZOOM_VARYING
        v_modelpos_base_zoom = modelPositionBaseZoom();
    #endif

    v_normal = normalize(u_normal_matrix * vec3(0., 0., 1.));

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

    #if defined(TANGRAM_LIGHTING_VERTEX)
        // Modify normal before lighting
        vec3 normal = v_normal;
        #pragma tangram: normal

        v_lighting = calculateLighting(v_position.xyz, normal, vec4(1.));
    #endif

    gl_Position = u_proj * v_position;
}
