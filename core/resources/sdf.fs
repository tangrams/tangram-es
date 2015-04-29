#ifdef GL_ES
precision mediump float;
#define LOWP lowp
#else
#define LOWP
#endif

#ifdef TANGRAM_SDF_MULTISAMPLING
    #ifdef GL_OES_standard_derivatives
        #extension GL_OES_standard_derivatives : enable
    #else
        #undef TANGRAM_SDF_MULTISAMPLING
    #endif
#endif

uniform sampler2D u_tex;
uniform LOWP vec3 u_color;

varying vec2 v_uv;
varying float v_alpha;

const float gamma = 2.2;
const float tint = 1.8;
const float sdf = 0.8;

float contour(in float d, in float w, in float off) {
    return smoothstep(off - w, off + w, d);
}

float sample(in vec2 uv, float w, in float off) {
    return contour(texture2D(u_tex, uv).a, w, off);
}

float sampleAlpha(in vec2 uv, float distance, in float off) {
    float alpha = contour(distance, distance, off);

    #ifdef TANGRAM_SDF_MULTISAMPLING
        float dscale = 0.354; // 1 / sqrt(2)
        vec2 duv = dscale * (dFdx(uv) + dFdy(uv));
        vec4 box = vec4(uv - duv, uv + duv);

        float asum = sample(box.xy, distance, off)
                   + sample(box.zw, distance, off)
                   + sample(box.xw, distance, off)
                   + sample(box.zy, distance, off);

        alpha = (alpha + 0.5 * asum) / 2.0;
    #endif

    return alpha;
}

void main(void) {
    if (v_alpha == 0.0) {
        discard;
    }

    float distance = texture2D(u_tex, v_uv).a;
    float alpha = sampleAlpha(v_uv, distance, sdf) * tint;
    alpha = pow(alpha, 1.0 / gamma);

    gl_FragColor = vec4(u_color, v_alpha * alpha);
}

