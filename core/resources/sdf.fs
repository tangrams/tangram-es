#ifdef GL_ES
precision mediump float;
#endif

uniform sampler2D u_tex;
uniform lowp vec3 u_color;

varying vec2 v_uv;
varying float v_alpha;

float minOutlineD = 0.25;
float maxOutlineD = 0.35;
float minInsideD = 0.38;
float maxInsideD = 0.55;
float mixFactor = 0.7;
const vec3 outlineColor = vec3(0.7);

const float gamma = 2.2;

void main(void) {
    float distance = texture2D(u_tex, v_uv).a;

    float a1 = smoothstep(minInsideD, maxInsideD, distance);
    float a2 = smoothstep(minOutlineD, maxOutlineD, distance);

    a1 = pow(a1, 1.0 / gamma);
    a2 = pow(a2, 1.0 / gamma);

    float d = clamp(distance / (1.0 - distance), 0.0, 1.0);

    gl_FragColor = vec4(mix(outlineColor * a2, u_color * a1, mixFactor), v_alpha * d);
}
