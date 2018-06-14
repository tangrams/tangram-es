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
varying float v_aa_factor;
varying vec2 v_edge;
varying vec4 v_outline_color;


uniform sampler2D u_tex;
uniform LOWP int u_sprite_mode;

#pragma tangram: global

void main(void) {

    vec4 color = v_color;

    if (u_sprite_mode == 0) {
        float point_dist = length(v_texcoords);

        if (v_outline_color.a > 0.0) {

          float outline_edge = v_edge.x;
          float fill_edge = v_edge.y;

          vec4 mixColor = mix(color, v_outline_color, v_outline_color.a);

          color = mix(color, mixColor,
                      smoothstep(max(0.0, outline_edge - v_aa_factor),
                                 min(1.0, outline_edge + v_aa_factor),
                                 point_dist));

          color = mix(color, v_outline_color,
                      smoothstep(max(0.0, fill_edge - v_aa_factor),
                                 min(1.0, fill_edge + v_aa_factor),
                                 point_dist));
        }

        color.a = mix(color.a, 0., (smoothstep(max(1. - v_aa_factor, 0.), 1., point_dist)));
    } else {
        color *= texture2D(u_tex, v_texcoords);
    }
    color.a *= v_alpha;

    #pragma tangram: color
    #pragma tangram: filter

    gl_FragColor = color;
}
