#ifdef GL_ES
precision mediump float;
#define LOWP lowp
#else
#define LOWP 
#endif

uniform sampler2D u_tex;
uniform LOWP vec3 u_color;

varying vec2 v_uv;
varying float v_alpha;

void main(void) {
    if (v_alpha == 0.0) {
        discard;
    }
    
    vec4 texColor = texture2D(u_tex, v_uv);
    gl_FragColor = vec4(u_color.rgb, texColor.a * v_alpha);
}
