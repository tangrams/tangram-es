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

// TODO: use this as attribute
const float textSize = 15.0;
const float emSize = textSize / 16.0;

uniform sampler2D u_tex;

varying vec2 v_uv;
varying float v_alpha;
varying vec4 v_color;

float contour(in float d, in float w) {
    return smoothstep(0.5 - w, 0.5 + w, d);
}

float sample(in vec2 uv, float w) {
    return contour(texture2D(u_tex, uv).a, w);
}

float sampleAlpha(in vec2 uv, float distance) {
    const float smooth = 0.0625 * emSize; // 0.0625 = 1.0/1em ratio
    float alpha = contour(distance, smooth);

#ifdef TANGRAM_SDF_MULTISAMPLING
    const float aaSmooth = smooth / 2.0;
    float dscale = 0.354; // 1 / sqrt(2)
    vec2 duv = dscale * (dFdx(uv) + dFdy(uv));
    vec4 box = vec4(uv - duv, uv + duv);

    float asum = sample(box.xy, aaSmooth)
        + sample(box.zw, aaSmooth)
        + sample(box.xw, aaSmooth)
        + sample(box.zy, aaSmooth);

    alpha = mix(alpha, asum, 0.25);
#endif

    return alpha;
}

void main(void) {
    if (v_alpha < TANGRAM_EPSILON) {
        discard;
    } else {
        float distance = texture2D(u_tex, v_uv).a;

        float alpha = sampleAlpha(v_uv, distance);
        alpha = pow(alpha, 0.4545);

        gl_FragColor = vec4(v_color.rgb, v_alpha * alpha * v_color.a);
    }
}

