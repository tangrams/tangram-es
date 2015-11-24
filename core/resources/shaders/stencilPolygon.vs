#ifdef GL_ES
precision highp float;
#endif

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_proj;
uniform vec3 u_map_position;
//uniform vec3 u_tile_origin;

attribute vec4 a_position;
//attribute float a_layer;

varying vec4 v_world_position;

void main() {
    // World coordinates for 3d procedural textures
    vec4 local_origin = vec4(u_map_position.xy, 0., 0.);
    #ifdef TANGRAM_WORLD_POSITION_WRAP
        local_origin = mod(local_origin, TANGRAM_WORLD_POSITION_WRAP);
    #endif
    v_world_position = a_position + local_origin;

    gl_Position = u_proj * u_view * u_model * a_position;

    // Proxy tiles have u_tile_origin.z < 0, so this adjustment will place proxy tiles
    // deeper in the depth buffer than non-proxy tiles
    // gl_Position.z += TANGRAM_DEPTH_DELTA * gl_Position.w * (1. - sign(u_tile_origin.z));

    // #ifdef TANGRAM_DEPTH_DELTA
    //     gl_Position.z -= a_layer * TANGRAM_DEPTH_DELTA * gl_Position.w;
    // #endif
}
