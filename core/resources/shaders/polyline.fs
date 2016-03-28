#pragma tangram: extensions

#ifdef GL_ES
precision highp float;
#endif

#pragma tangram: defines

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;
uniform mat3 u_normal_matrix;
uniform mat3 u_inverse_normal_matrix;
uniform vec4 u_tile_origin;
uniform vec3 u_map_position;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_meters_per_pixel;
uniform float u_device_pixel_ratio;

#pragma tangram: uniforms

varying vec4 v_world_position;
varying vec4 v_position;
varying vec4 v_color;
varying vec3 v_normal;

#ifdef TANGRAM_USE_TEX_COORDS
    varying vec2 v_texcoord;
#endif

#ifdef TANGRAM_LIGHTING_VERTEX
    varying vec4 v_lighting;
#endif

vec3 worldNormal() {
    return normalize(u_inverse_normal_matrix * v_normal);
}

vec4 worldPosition() {
    return v_world_position;
}

#pragma tangram: material
#pragma tangram: lighting
#pragma tangram: global

void main(void) {

    // Initialize globals
    #pragma tangram: setup

    vec4 color = v_color;
    vec3 normal = v_normal;

    // Modify material properties before lighting
    #ifdef TANGRAM_MATERIAL_NORMAL_TEXTURE
        calculateNormal(normal);
    #endif

    #ifdef TANGRAM_LIGHTING_FRAGMENT
        // Modify normal and color before lighting
        #pragma tangram: normal

        #pragma tangram: color

        color = calculateLighting(v_position.xyz, normal, color);
    #else
        #ifdef TANGRAM_LIGHTING_VERTEX
            color = v_lighting;
        #endif
    #endif

    // Modify color after lighting (filter-like effects that don't require a additional render passes)
    #pragma tangram: filter

    gl_FragColor = color;
}
