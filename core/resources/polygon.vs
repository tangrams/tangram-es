uniform mat4 u_model;
uniform mat4 u_modelView;
uniform mat4 u_modelViewProj;
uniform float u_time;

#pragma tangram: lighting

attribute vec4 a_position;
attribute vec4 a_color;
attribute vec3 a_normal;
attribute vec2 a_texcoord;

varying vec4 v_color;
varying vec3 v_eye;
varying vec3 v_normal;
varying vec2 v_texcoord;

void main() {

  	v_normal = normalize(a_normal);
  	v_texcoord = a_texcoord;

	// v_eye = vec3(a_position);
	v_eye = vec3(u_modelView * a_position);

	v_color = a_color; // * calculateLighting(v_eye,v_normal);

  	gl_Position = u_modelViewProj * a_position;
}
