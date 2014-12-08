#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 u_model;
uniform mat4 u_modelView;
uniform mat4 u_modelViewProj;
uniform float u_time;

#pragma tangram: lighting

varying vec4 v_color;
varying vec3 v_eye;
varying vec3 v_normal;
varying vec2 v_texcoord;

void main(void) {
	vec4 color = v_color;

	color *= calculateLighting(v_eye,v_normal);

  	gl_FragColor = color;
}
