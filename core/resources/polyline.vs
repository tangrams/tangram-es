#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 u_modelViewProj;
uniform vec3 u_lightDirection;
uniform float u_time;
uniform float u_tileDepthOffset;

attribute vec4 a_position;
attribute vec2 a_texcoord;

attribute vec3 a_extrudeNormal;
attribute float a_extrudeWidth;

attribute vec4 a_color;

varying vec4 v_pos;
varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_texcoord;

void main() {

	v_normal = vec3(0.,0.,1.);

  	v_color = a_color;

  	float lit = dot(normalize(u_lightDirection), normalize(v_normal));
  	v_color.rgb *= clamp(lit * 1.5, 0.5, 1.5);

  	v_texcoord = a_texcoord;
  	
  	v_pos = a_position;
  	v_pos.xyz += a_extrudeNormal*a_extrudeWidth;

	gl_Position = u_modelViewProj * v_pos;
    
    gl_Position.z /= u_tileDepthOffset;
}
