#ifdef GL_ES
precision mediump float;
#endif

uniform lowp sampler2D u_tex;
uniform lowp vec4 u_color;
uniform lowp vec4 u_outlineColor;
uniform lowp vec4 u_sdfParams;
uniform lowp float u_mixFactor;

#define minOutlineD u_sdfParams.x
#define maxOutlineD u_sdfParams.y
#define minInsideD  u_sdfParams.z
#define maxInsideD  u_sdfParams.w

varying vec2 f_uv;

void main(void) {
    float distance = texture2D(u_tex, f_uv).a;
    vec4 inside = smoothstep(minInsideD, maxInsideD, distance) * u_color;
    vec4 outline = smoothstep(minOutlineD, maxOutlineD, distance) * u_outlineColor;

    gl_FragColor = mix(outline, inside, u_mixFactor);
}
