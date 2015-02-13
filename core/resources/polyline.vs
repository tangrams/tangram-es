#ifdef GL_ES
precision highp float;
#endif

uniform mat4 u_modelView;
uniform mat4 u_modelViewProj;
uniform float u_time;
uniform float u_tileDepthOffset;

attribute vec4 a_position;
attribute vec4 a_color;

attribute vec3 a_extrudeNormal;
attribute float a_extrudeWidth;

attribute vec3 a_normal;
attribute vec2 a_texcoord;

varying vec4 v_color;
varying vec3 v_eyeToPoint;
varying vec3 v_normal;
varying vec2 v_texcoord;

void main() {

	v_normal = vec3(0.,0.,1.);

	v_color = a_color;
  	v_texcoord = a_texcoord;

	vec4 v_pos = a_position;
  	v_pos.xyz += a_extrudeNormal * a_extrudeWidth;

	v_eyeToPoint = vec3(u_modelView * a_position);

	gl_Position = u_modelViewProj * v_pos;
    gl_Position.z /= u_tileDepthOffset;
}
