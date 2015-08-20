#ifdef GL_ES
precision mediump float;
#define LOWP lowp
#else
#define LOWP
#endif

attribute LOWP vec2 a_position;
attribute LOWP vec2 a_screenPosition;
attribute vec2 a_uv;
attribute LOWP float a_alpha;
attribute LOWP float a_rotation;
attribute LOWP vec3 a_color;

uniform mat4 u_proj;

varying vec2 v_uv;
varying float v_alpha;
varying vec3 v_color;

void main() {
    if (a_alpha > TANGRAM_EPSILON) {
        float st = sin(a_rotation);
        float ct = cos(a_rotation);

        // rotates first around +z-axis (0,0,1) and then translates by (tx,ty,0)
        vec4 p = vec4(
            a_position.x * ct - a_position.y * st + a_screenPosition.x,
            a_position.x * st + a_position.y * ct + a_screenPosition.y,
            0.0, 1.0
        );

        gl_Position = u_proj * p;
    } else {
        gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
    }

    v_alpha = a_alpha;
    v_uv = a_uv;
	v_color = a_color;
}
