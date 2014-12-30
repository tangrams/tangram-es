#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 u_modelViewProj;
uniform vec3 u_lightDirection;
uniform float u_proxyOffset;

attribute vec4 a_position;
attribute vec4 a_color;
attribute vec3 a_normal;
attribute vec2 a_texcoord;

varying vec4 v_pos;
varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_texcoord;

void main() {

	v_normal = a_normal;

  	v_color = a_color;
  	
  	float lit = dot(normalize(u_lightDirection), normalize(a_normal));
  	v_color.rgb *= clamp(lit * 1.5, 0.5, 1.5);
  	
	v_texcoord = a_texcoord;

  	gl_Position = u_modelViewProj * a_position;
    
    gl_Position.z /= u_proxyOffset;
}
