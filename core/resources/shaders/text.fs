#ifdef GL_ES
precision mediump float;
#define LOWP lowp
#else
#define LOWP 
#endif

uniform sampler2D u_tex;

varying vec2 v_uv;
varying float v_alpha;
varying vec4 v_color;

void main(void) {
    if (v_alpha < TANGRAM_EPSILON) {
        discard;
    } else {
        vec4 texColor = texture2D(u_tex, v_uv);
        gl_FragColor = vec4(v_color.rgb, texColor.a * v_alpha * v_color.a);
    }
}
