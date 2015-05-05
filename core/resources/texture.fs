#ifdef GL_ES
precision mediump float;
#define LOWP lowp
#else
#define LOWP
#endif

uniform sampler2D u_tex;

varying vec2 v_uv;

void main(void) {
    gl_FragColor = texture2D(u_tex, v_uv);
}
