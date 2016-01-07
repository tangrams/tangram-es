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

#pragma tangram: uniforms

varying vec4 v_color;
varying vec4 v_strokeColor;
varying vec2 v_texcoords;
varying float v_strokeWidth;
varying float v_alpha;

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

    float threshold_fill = 0.5;
    float threshold_stroke = threshold_fill - v_strokeWidth * u_device_pixel_ratio;

    float distance = texture2D(u_tex, v_texcoords).a;

    float alpha_fill = pow(sampleAlpha(v_texcoords, distance, threshold_fill), 0.4545);
    float alpha_stroke = pow(sampleAlpha(v_texcoords, distance, threshold_stroke), 0.4545);

    vec4 color = mix(v_strokeColor, v_color, alpha_fill);
    color.a = max(alpha_fill, alpha_stroke) * v_alpha * v_color.a;

    #pragma tangram: color
    #pragma tangram: filter

    gl_FragColor = color;
}

