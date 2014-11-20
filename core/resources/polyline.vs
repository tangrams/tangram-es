#ifdef GL_ES
precision mediump float;
#endif

//	lights.glsl
//
#define NUM_LIGHTS 4
uniform struct Light {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
   	vec4 position;
   	vec4 halfVector;
   	vec3 direction;
   	float spotExponent;
    float spotCutoff;
	float spotCosCutoff;
    float constantAttenuation;
    float linearAttenuation;
    float quadraticAttenuation;
} u_lights[NUM_LIGHTS];

uniform struct Material {
	vec4 emission;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
} u_material;

uniform mat4 u_modelViewProj;
uniform float u_time;

attribute vec4 a_position;
attribute vec2 a_uv;

attribute vec3 a_extrudeNormal;
attribute float a_extrudeWidth;

attribute vec4 a_color;

varying vec4 v_pos;
varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_uv;

void main() {

	v_normal = vec3(0.,0.,1.);

  	v_color = a_color;

  	float lit = dot(normalize(u_lights[0].direction), normalize(v_normal));
  	v_color.rgb *= clamp(lit * 1.5, 0.5, 1.5);

  	v_uv = a_uv;
  	
  	v_pos = a_position;
  	v_pos.xyz += a_extrudeNormal*a_extrudeWidth;

	gl_Position = u_modelViewProj * v_pos;
}