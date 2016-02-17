#pragma tangram: extensions

#ifdef GL_ES
precision mediump float;
#define LOWP lowp
#else
#define LOWP
#endif

#pragma tangram: defines

uniform mat4 u_ortho;
uniform vec3 u_map_position;
uniform vec3 u_tile_origin;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_meters_per_pixel;
uniform float u_device_pixel_ratio;
#ifdef TANGRAM_TEXT
uniform vec2 u_uv_scale_factor;
uniform int u_pass;
#endif

#pragma tangram: uniforms

attribute vec2 a_uv;
attribute vec2 a_position;
attribute vec2 a_screenPosition;
attribute LOWP float a_alpha;
attribute LOWP float a_rotation;
attribute LOWP vec4 a_color;
#ifdef TANGRAM_TEXT
attribute LOWP vec4 a_stroke;
attribute float a_scale;
#else
attribute vec3 a_extrude;
#endif

varying vec4 v_color;
varying vec2 v_texcoords;
#ifdef TANGRAM_TEXT
varying float v_sdf_threshold;
varying float v_sdf_scale;
#endif
varying float v_alpha;
const vec4 clipped = vec4(2.0, 0.0, 2.0, 1.0);

#pragma tangram: global

#define UNPACK_POSITION(x) (x / 4.0) // 4 subpixel precision
#define UNPACK_EXTRUDE(x) (x / 256.0)
#define UNPACK_ROTATION(x) (x / 4096.0)

void main() {

    if (a_alpha > TANGRAM_EPSILON) {

        v_alpha = a_alpha;
        v_color = a_color;

        vec2 vertexPos = UNPACK_POSITION(a_position);

        #ifdef TANGRAM_TEXT
        v_texcoords = a_uv * u_uv_scale_factor;
        v_sdf_scale = a_scale / 64.0;

        if (u_pass == 0) {
            // fill
            v_sdf_threshold = 0.5;
        } else {
            // stroke
            // (0.5 / 3.0) <= sdf change per pixel == 0.083
            float sdf_pixel = 0.5/3.0;

            // de-normalize (*3.0)
            float stroke_width = a_stroke.a * 3.0;

            // scale to sdf pixel
            stroke_width *= sdf_pixel;

            // scale sdf (texture is scaled depeding on font size)
            stroke_width /= v_sdf_scale;

            v_sdf_threshold = 0.5 - stroke_width;

            v_color.rgb = a_stroke.rgb;
        }

        #else
        v_texcoords = a_uv;
        if (a_extrude.x != 0.0) {
            float dz = u_map_position.z - abs(u_tile_origin.z);
            vertexPos.xy += clamp(dz, 0.0, 1.0) * UNPACK_EXTRUDE(a_extrude.xy);
        }
        #endif

        // rotates first around +z-axis (0,0,1) and then translates by (tx,ty,0)
        float st = sin(UNPACK_ROTATION(a_rotation));
        float ct = cos(UNPACK_ROTATION(a_rotation));
        vec2 screenPos = UNPACK_POSITION(a_screenPosition);
        vec4 position = vec4(
            vertexPos.x * ct - vertexPos.y * st + screenPos.x,
            vertexPos.x * st + vertexPos.y * ct + screenPos.y,
            0.0, 1.0
        );

        #pragma tangram: position

        gl_Position = u_ortho * position;

    } else {
        gl_Position = clipped;
    }

}
