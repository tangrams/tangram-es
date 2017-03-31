#pragma tangram: extensions

#ifdef GL_ES
precision mediump float;
#define LOWP lowp
#else
#define LOWP
#endif

#pragma tangram: defines

uniform mat4 u_ortho;
uniform vec4 u_tile_origin;
uniform vec3 u_map_position;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_meters_per_pixel;
uniform float u_device_pixel_ratio;
#ifdef TANGRAM_TEXT
uniform vec2 u_uv_scale_factor;
uniform float u_max_stroke_width;
uniform LOWP int u_pass;
#endif

#pragma tangram: uniforms

attribute vec2 a_uv;
attribute LOWP float a_alpha;
attribute LOWP vec4 a_color;
#ifdef TANGRAM_TEXT
attribute vec2 a_position;
attribute LOWP vec4 a_stroke;
attribute float a_scale;
#else
attribute vec3 a_position;
#endif

#ifdef TANGRAM_FEATURE_SELECTION
attribute vec4 a_selection_color;
varying vec4 v_selection_color;
#endif

varying vec4 v_color;
varying vec2 v_texcoords;
#ifdef TANGRAM_TEXT
varying float v_sdf_threshold;
varying float v_sdf_pixel;
#endif
varying float v_alpha;

#pragma tangram: global

#define UNPACK_POSITION(x) (x / 4.0) // 4 subpixel precision
#define UNPACK_EXTRUDE(x) (x / 256.0)
#define UNPACK_TEXTURE(x) (x * u_uv_scale_factor)

void main() {

    v_alpha = a_alpha;
    v_color = a_color;

#ifdef TANGRAM_FEATURE_SELECTION
    v_selection_color = a_selection_color;
    // Skip non-selectable meshes
    if (v_selection_color == vec4(0.0)) {
        gl_Position = vec4(0.0);
        return;
    }
#endif

#ifdef TANGRAM_TEXT
    vec2 vertex_pos = UNPACK_POSITION(a_position);
    v_texcoords = UNPACK_TEXTURE(a_uv);
    float sdf_scale = a_scale / 64.0;
    v_sdf_pixel = 0.5 / (u_max_stroke_width * sdf_scale);

    if (u_pass == 0) {
        v_sdf_threshold = 0.5;
    } else {
        if ((a_stroke.a > 0.0)) {
            float stroke_width;
            stroke_width = ((a_stroke.a * u_max_stroke_width) * (0.5 / u_max_stroke_width));
            stroke_width = (stroke_width / sdf_scale);
            v_sdf_threshold = max ((0.5 - stroke_width), 0.0);
            v_color.xyz = a_stroke.xyz;
        } else {
            v_alpha = 0.0;
        }
    }

    vec4 position;
    position.zw = vec2(0.0, 1.0);
    position.xy = vertex_pos;

    #pragma tangram: position

    gl_Position = (u_ortho * position);

#else
    v_texcoords = a_uv;

    gl_Position = vec4(a_position, 1.0);

#endif

}
