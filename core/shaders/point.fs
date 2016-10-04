#pragma tangram: extensions

#ifdef GL_ES
precision mediump float;
#define LOWP lowp
#else
#define LOWP
#endif

#pragma tangram: defines

uniform vec4 u_tile_origin;
uniform vec3 u_map_position;
uniform vec2 u_resolution;
uniform float u_time;
uniform float u_meters_per_pixel;
uniform float u_device_pixel_ratio;

#pragma tangram: uniforms

varying vec4 v_color;
varying vec2 v_texcoords;
varying float v_alpha;

uniform sampler2D u_tex;

#pragma tangram: global

void main(void) {
    if (v_alpha < TANGRAM_EPSILON) {
        discard;
    } else {
        vec4 texColor = texture2D(u_tex, v_texcoords);
        vec4 color = vec4(texColor.rgb * v_color.rgb, v_alpha * texColor.a * v_color.a);

        #pragma tangram: color
        #pragma tangram: filter

        gl_FragColor = color;
    }
}
