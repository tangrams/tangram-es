#ifdef GL_ES
precision highp float;
#endif

uniform mat4 u_modelViewProj;

attribute vec3 a_position;

varying vec3 v_uv;

const mat3 rotNegHalfPiAroundX = mat3( 1.0,  0.0,  0.0,
        0.0,  0.0, -1.0,
        0.0,  1.0,  0.0);

void main() {
    // The map coordinates use +z as "up" instead of the cubemap convention of +y,
    // so we rotate the texture coordinates by pi/2 around x to correct the orientation
    v_uv = rotNegHalfPiAroundX * vec3(a_position);
    vec4 pos = u_modelViewProj * vec4(a_position, 1.0);

    // force depth to 1.0
    gl_Position = pos.xyww;
}
