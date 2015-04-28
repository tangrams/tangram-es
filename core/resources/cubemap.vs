#ifdef GL_ES
precision highp float;
#endif

uniform mat4 u_modelViewProj;

attribute vec3 a_position;

varying vec3 v_uv;

void main() {
    v_uv = a_position;
    vec4 pos = u_modelViewProj * vec4(a_position, 1);

    // force depth to 1.0
	gl_Position = pos.xyww;
}
