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

#ifndef TANGRAM_POINT
uniform sampler2D u_tex;
#else
const float borderWidth = 0.3;
const float circleRadiusIn = 0.5;
const float circleRadiusOut = circleRadiusIn + borderWidth;

float circle(vec2 r, vec2 center, float radius) {
    return 1.0 - smoothstep(radius - 0.2, radius + 0.2, length(r - center));
}
#endif

#pragma tangram: global

void main(void) {
    if (v_alpha < TANGRAM_EPSILON) {
        discard;
    } else {
        vec4 color;

        #ifdef TANGRAM_POINT
            vec2 uv = v_texcoords * 2.0 - 1.0;
            float c1 = circle(uv, vec2(0.0), circleRadiusIn);
            float c2 = circle(uv, vec2(0.0), circleRadiusOut);
            color = vec4(vec3(c1) * v_color.rgb, c2 * v_alpha * v_color.a);
        #else
            vec4 texColor = texture2D(u_tex, v_texcoords);
            color = vec4(texColor.rgb * v_color.rgb, v_alpha * texColor.a * v_color.a);
        #endif

        #pragma tangram: color
        #pragma tangram: filter

        gl_FragColor = color;
    }
}
