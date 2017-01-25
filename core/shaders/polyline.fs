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
uniform float u_texture_ratio;
uniform sampler2D u_texture;

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

vec4 worldPosition() {
    return v_world_position;
}

vec3 worldNormal() {
    return normalize(u_inverse_normal_matrix * v_normal);
}

#pragma tangram: material
#pragma tangram: lighting
#pragma tangram: global
#pragma tangram: raster

#ifdef TANGRAM_MODEL_POSITION_BASE_ZOOM_VARYING
    varying vec4 v_modelpos_base_zoom;
#endif

void main(void) {

    // Initialize globals
    #pragma tangram: setup

    vec4 color = v_color;
    vec3 normal = v_normal;

    #ifdef TANGRAM_RASTER_TEXTURE_COLOR
        color *= sampleRaster(0);
    #endif

    #ifdef TANGRAM_LINE_TEXTURE
        vec2 line_st = vec2(v_texcoord.x, fract(v_texcoord.y * TANGRAM_DASHLINE_TEX_SCALE / u_texture_ratio));
        vec4 line_color = texture2D(u_texture, line_st);

        if (line_color.a < TANGRAM_ALPHA_TEST) {
            #ifdef TANGRAM_LINE_BACKGROUND_COLOR
                color.rgb = TANGRAM_LINE_BACKGROUND_COLOR;
            #elif !defined(TANGRAM_BLEND_OVERLAY) && !defined(TANGRAM_BLEND_INLAY)
                discard;
            #else
                color.a = 0.0;
            #endif
        } else {
            color *= line_color;
        }
    #endif

    #ifdef TANGRAM_RASTER_TEXTURE_NORMAL
        normal = normalize(sampleRaster(0).rgb * 2.0 - 1.0);
    #endif

    #ifdef TANGRAM_MATERIAL_NORMAL_TEXTURE
        calculateNormal(normal);
    #endif

    // Modify normal before lighting if not already modified in vertex shader
    #if !defined(TANGRAM_LIGHTING_VERTEX)
        #pragma tangram: normal
    #endif

    // Modify color before lighting is applied
    #pragma tangram: color

    #if defined(TANGRAM_LIGHTING_FRAGMENT)
        color = calculateLighting(v_position.xyz, normal, color);
    #elif defined(TANGRAM_LIGHTING_VERTEX)
        color *= v_lighting;
    #endif

    // Modify color after lighting (filter-like effects that don't require a additional render passes)
    #pragma tangram: filter

    gl_FragColor = color;
}
