#pragma tangram: extensions

#ifdef GL_ES
    precision mediump float;
    #define LOWP lowp
#else
    #define LOWP
#endif

#pragma tangram: defines

uniform sampler2D u_tex;
uniform vec3 u_map_position;
uniform vec4 u_tile_origin;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_meters_per_pixel;
uniform float u_device_pixel_ratio;
uniform float u_max_stroke_width;
uniform LOWP int u_pass;

#pragma tangram: uniforms

varying vec4 v_color;
varying vec2 v_texcoords;
varying float v_sdf_threshold;
varying float v_alpha;
varying float v_sdf_scale;

#pragma tangram: global

void main(void) {

    vec4 color = v_color;

    float signed_distance = texture2D(u_tex, v_texcoords).a;

    // - At the glyph outline alpha is 0.5
    //
    // - The sdf-radius is 3.0px, i.e. within 3px distance
    //   from the outline alpha is in the range (0.5 -> 0.0)
    //
    // - 0.5 pixel threshold (to both sides of the outline)
    //   plus 0.25 for a bit of smoothness
    //
    //   ==> (0.5 / 3.0) * (0.5 + 0.25) == 0.1245
    //   This value is added to sdf_threshold to antialias
    //   the outline within one pixel for the *unscaled* glyph.
    //
    // - sdf_scale == fontScale / glyphScale:
    //   When the glyph is scaled down, 's' must be increased
    //   (used to interpolate 1px of the scaled glyph around v_sdf_threshold)

    float sdf_pixel = 0.5 / (u_max_stroke_width * v_sdf_scale);
    float add_smooth = 0.25;
    float filter_width = (sdf_pixel * (0.5 + add_smooth));

    float start = max(v_sdf_threshold - filter_width, 0.0);
    float end = v_sdf_threshold + filter_width;

    float alpha;

    if (u_pass == 0) {
        alpha = smoothstep(start, end, signed_distance);
    } else {
        // smooth the signed distance for outlines
        float signed_distance_1_over_2 = 1.0 / (2.0 * signed_distance);
        float smooth_signed_distance = pow(signed_distance, signed_distance_1_over_2);

        alpha = smoothstep(start, end, smooth_signed_distance);
    }

    color.a *= v_alpha * alpha;

    #pragma tangram: color
    #pragma tangram: filter

    gl_FragColor = color;
}
