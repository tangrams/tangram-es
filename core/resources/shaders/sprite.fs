#ifdef GL_ES
precision mediump float;
#define LOWP lowp
#else
#define LOWP
#endif

uniform sampler2D u_tex;

varying vec2 v_uv;
varying float v_alpha;
varying vec3 v_color;

void main(void) {
    if (v_alpha < TANGRAM_EPSILON) {
        discard;
    } else {
        vec4 color = texture2D(u_tex, v_uv);
		gl_FragColor = vec4(color.rgb * v_color, v_alpha * color.a);
    }
}
