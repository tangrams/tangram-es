#pragma tangram: extensions

#ifdef TANGRAM_SDF_MULTISAMPLING
    #ifdef GL_OES_standard_derivatives
        #extension GL_OES_standard_derivatives : enable
    #else
        #undef TANGRAM_SDF_MULTISAMPLING
    #endif
#endif

#ifdef GL_ES
    precision mediump float;
    #define LOWP lowp
#else
    #define LOWP
#endif

#pragma tangram: defines

const float emSize = 15.0 / 16.0;

uniform sampler2D u_tex;
uniform vec3 u_map_position;
uniform vec3 u_tile_origin;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_meters_per_pixel;
uniform float u_device_pixel_ratio;
uniform int u_pass;
uniform float u_max_stroke_width;

#pragma tangram: uniforms

varying vec4 v_color;
varying vec2 v_texcoords;
varying float v_sdf_threshold;
varying float v_alpha;
varying float v_sdf_scale;

#pragma tangram: global

float contour(in float d, in float w, float t) {
    return smoothstep(t - w, t + w, d);
}

float sample(in vec2 uv, float w, float t) {
    return contour(texture2D(u_tex, uv).a, w, t);
}

float sampleAlpha(in vec2 uv, float distance, float threshold) {
    const float smoothing = 0.0625 * emSize; // 0.0625 = 1.0/1em ratio
    float alpha = contour(distance, smoothing, threshold);

#ifdef TANGRAM_SDF_MULTISAMPLING
    const float aaSmooth = smoothing / 2.0;
    float dscale = 0.354; // 1 / sqrt(2)
    vec2 duv = dscale * (dFdx(uv) + dFdy(uv));
    vec4 box = vec4(uv - duv, uv + duv);

    float asum = sample(box.xy, aaSmooth, threshold)
               + sample(box.zw, aaSmooth, threshold)
               + sample(box.xw, aaSmooth, threshold)
               + sample(box.zy, aaSmooth, threshold);

    alpha = mix(alpha, asum, 0.25);
#endif

    return alpha;
}

void main(void) {

    vec4 color = v_color;

    float dist = texture2D(u_tex, v_texcoords).a;

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
    //float filter_width = 0.1245 / v_sdf_scale;

    float sdf_pixel = (0.5/u_max_stroke_width) / v_sdf_scale;
    float add_smooth = 0.25;
    float filter_width = (sdf_pixel * (0.5 + add_smooth));

    float alpha = smoothstep(max(v_sdf_threshold - filter_width, 0.0),
                             v_sdf_threshold + filter_width,
                             dist);

    color.a *= v_alpha * alpha;

    //color = mix(vec4(1., 0., 0., 1.), vec4(0., 0., 1., 1.), dist);
    // color = mix(vec4(1., 0., 0., 1.), vec4(0., 0., 1., 1.), step(dist,0.5));
    // color.a *= max(alpha, 0.2);

    #pragma tangram: color
    #pragma tangram: filter

    gl_FragColor = color;
}
