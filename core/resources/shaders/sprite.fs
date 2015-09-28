#ifdef GL_ES
precision mediump float;
#define LOWP lowp
#else
#define LOWP
#endif

uniform sampler2D u_tex;
uniform vec3 u_map_position;
uniform vec3 u_tile_origin;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_meters_per_pixel;

varying vec2 v_uv;
varying float v_alpha;
varying vec4 v_color;

void main(void) {
    if (v_alpha < TANGRAM_EPSILON) {
        discard;
    } else {
        vec4 color = texture2D(u_tex, v_uv);
        gl_FragColor = vec4(color.rgb * v_color.rgb, v_alpha * color.a * v_color.a);
    }
}
