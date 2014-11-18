#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 u_modelViewProj;
uniform vec4 u_lightDirection;
uniform float u_time;

attribute vec4 a_position;
attribute vec3 a_normal;
attribute vec3 a_extrudeNormal;
attribute float a_extrudeWidth;
attribute vec4 a_color;

varying vec4 v_pos;
varying vec4 v_color;

void main() {

	vec4 normal = vec4(0.,0.,1.,0.);

  	float lit = dot(normalize(u_lightDirection), normalize(normal));
  	v_color = a_color;
  	v_color.rgb *= clamp(lit * 1.5, 0.5, 1.5);

  	v_pos = a_position;

  	v_pos.xyz += a_extrudeNormal*(a_extrudeWidth*abs(sin(u_time*2.)));
  	// pos.xyz += a_extrudeNormal*(a_extrudeWidth*abs(sin(u_time*4.*pos.x*pos.y)));

	gl_Position = u_modelViewProj * v_pos;
}