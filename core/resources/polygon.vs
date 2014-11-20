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

attribute vec4 a_position;
attribute vec4 a_color;
attribute vec3 a_normal;
attribute vec2 a_uv;

varying vec4 v_pos;
varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_uv;

void main() {

	v_normal = a_normal;

  	v_color = a_color;
  	
  	float lit = dot(normalize(u_lights[0].direction), normalize(a_normal));
  	v_color.rgb *= clamp(lit * 1.5, 0.5, 1.5);
  	
	v_uv = a_uv;

  	gl_Position = u_modelViewProj * a_position;
}
