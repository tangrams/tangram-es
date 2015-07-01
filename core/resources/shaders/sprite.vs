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

varying vec2 v_uv;
varying float v_alpha;

uniform mat4 u_proj;

void main(void) {
    if (a_alpha != 0.0) {
        float st = sin(a_rotation);
        float ct = cos(a_rotation);
        
        vec4 p = vec4(
            a_position.x * ct - a_position.y * st + a_screenPosition.x,
            a_position.x * st + a_position.y * ct + a_screenPosition.y,
            0.0, 1.0
        );
        
        gl_Position = u_proj * p;
        
        v_uv = a_uv;
        v_alpha = a_alpha;
    } else {
        // clip this vertex
        gl_Position = vec4(0.0);
    }
}
