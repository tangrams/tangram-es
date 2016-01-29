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

#pragma tangram: uniforms

varying vec4 v_color;
varying vec2 v_texcoords;
varying float v_sdf_threshold;
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

    vec4 color = v_color;

    //float distance = texture2D(u_tex, v_texcoords).a;
    // color *= v_alpha * pow(sampleAlpha(v_texcoords, distance, v_sdf_threshold), 0.4545);

    float dist = texture2D(u_tex, v_texcoords).a;

    // emSize 15/16 = .937
    // float s = 0.0625 * emSize; // 0.0625 = 1.0/1em ratio
    // // ==> .0666
    float s = 0.066;
    s *= 2.5;

    float alpha = smoothstep(v_sdf_threshold - s,
                             v_sdf_threshold + s,
                             dist);

    color *= v_alpha * alpha;


    #pragma tangram: color
    #pragma tangram: filter

    gl_FragColor = color;
}
