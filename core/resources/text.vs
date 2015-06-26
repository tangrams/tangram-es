#ifdef GL_ES
precision mediump float;
#define LOWP lowp
#else
#define LOWP
#endif

attribute vec2 a_position;
attribute vec2 a_screenPosition;
attribute vec2 a_uvs;
attribute float a_alpha;
attribute float a_rotation;

uniform mat4 u_proj;

varying vec2 v_uv;
varying float v_alpha;

void main() {
    if (a_alpha != 0.0) {
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
        gl_Position = vec4(0.0);
    }
    
    v_uv = a_uvs;
    v_alpha = a_alpha;
}
