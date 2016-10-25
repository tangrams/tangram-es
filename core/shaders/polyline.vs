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
attribute vec4 a_extrude;

#ifdef TANGRAM_USE_TEX_COORDS
    attribute vec2 a_texcoord;
    varying vec2 v_texcoord;
#endif

#ifdef TANGRAM_FEATURE_SELECTION
    // Make sure lighting is a no-op for feature selection pass
    #undef TANGRAM_LIGHTING_VERTEX

    attribute vec4 a_selection_color;
    varying vec4 v_selection_color;
#endif

varying vec4 v_world_position;
varying vec4 v_position;
varying vec4 v_color;
varying vec3 v_normal;

#ifdef TANGRAM_LIGHTING_VERTEX
    varying vec4 v_lighting;
#endif

#define UNPACK_POSITION(x) (x / 8192.0)
#define UNPACK_EXTRUSION(x) (x / 4096.0)
#define UNPACK_ORDER(x) (x / 2.0)
#define UNPACK_TEXCOORD(x) (x / 8192.0)

vec4 modelPosition() {
    return vec4(UNPACK_POSITION(a_position.xyz) * exp2(u_tile_origin.z - u_tile_origin.w), 1.0);
}

vec4 worldPosition() {
    return v_world_position;
}

vec3 worldNormal() {
    return vec3(0.0, 0.0, 1.0);
}

vec4 modelPositionBaseZoom() {
    return vec4(UNPACK_POSITION(a_position.xyz), 1.0);
}

#pragma tangram: material
#pragma tangram: lighting
#pragma tangram: global
#pragma tangram: raster

#ifdef TANGRAM_MODEL_POSITION_BASE_ZOOM_VARYING
    varying vec4 v_modelpos_base_zoom;
#endif

void main() {

    vec4 position = vec4(UNPACK_POSITION(a_position.xyz), 1.0);

    #ifdef TANGRAM_FEATURE_SELECTION
        v_selection_color = a_selection_color;
        // Skip non-selectable meshes
        if (v_selection_color == vec4(0.0)) {
            gl_Position = vec4(0.0);
            return;
        }
    #else
        // Initialize globals
        #pragma tangram: setup
    #endif

    v_color = a_color;

    #ifdef TANGRAM_USE_TEX_COORDS
        v_texcoord = UNPACK_TEXCOORD(a_texcoord);
    #endif

    #ifdef TANGRAM_MODEL_POSITION_BASE_ZOOM_VARYING
        v_modelpos_base_zoom = modelPositionBaseZoom();
    #endif

    v_normal = u_normal_matrix * vec3(0.,0.,1.);

    {
        vec4 extrude = UNPACK_EXTRUSION(a_extrude);
        float width = extrude.z;
        float dwdz = extrude.w;
        float dz = u_map_position.z - u_tile_origin.z;

        // Interpolate between zoom levels
        width += dwdz * clamp(dz, 0.0, 1.0);
        // Scale pixel dimensions to be consistent in screen space
        // and adjust scale for overzooming.
        width *= exp2(-dz + (u_tile_origin.w - u_tile_origin.z));

        // Modify line width in model space before extrusion
        #pragma tangram: width

        #ifdef TANGRAM_USE_TEX_COORDS
            v_texcoord.y /= 2. * extrude.z;
        #endif

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
        // Modify normal before lighting
        vec3 normal = v_normal;
        #pragma tangram: normal

        v_lighting = calculateLighting(v_position.xyz, normal, vec4(1.));
    #endif

    gl_Position = u_proj * v_position;

    // Proxy tiles are placed deeper in the depth buffer than non-proxy tiles
    gl_Position.z += TANGRAM_DEPTH_DELTA * gl_Position.w * u_proxy_depth;

    #ifdef TANGRAM_DEPTH_DELTA
        float layer = UNPACK_ORDER(a_position.w);
        gl_Position.z -= layer * TANGRAM_DEPTH_DELTA * gl_Position.w;
    #endif
}
