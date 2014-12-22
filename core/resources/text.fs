#ifdef GL_ES
precision mediump float;
#endif

uniform lowp sampler2D u_tex;
uniform lowp vec3 u_color;

varying vec2 f_uv;
varying float f_alpha;

void main(void) {
    vec4 texColor = texture2D(u_tex, f_uv);
    gl_FragColor = vec4(u_color.rgb, texColor.a * f_alpha);
}
