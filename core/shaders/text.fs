#pragma tangram: extensions

#ifdef GL_ES
precision mediump float;
#define LOWP lowp
#else
#define LOWP
#endif

#pragma tangram: defines

uniform sampler2D u_tex;
uniform vec3 u_map_position;
uniform vec4 u_tile_origin;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_meters_per_pixel;
uniform float u_device_pixel_ratio;
uniform vec2 u_uv_scale_factor;

#pragma tangram: uniforms

varying vec2 v_uv;
varying float v_alpha;
varying vec4 v_color;

#pragma tangram: global

void main(void) {
    if (v_alpha < TANGRAM_EPSILON) {
        discard;
    } else {
        vec4 color;
        vec4 texColor = texture2D(u_tex, v_uv * u_uv_scale_factor);

        color = vec4(v_color.rgb, texColor.a * v_alpha * v_color.a);

        #pragma tangram: color
        #pragma tangram: filter

        gl_FragColor = color;
    }
}
