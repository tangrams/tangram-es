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

uniform float u_time;

varying vec4 v_pos;
varying vec4 v_color;
varying vec2 v_uv;

void main(void) {
	vec4 color = v_color;
	
  	gl_FragColor = color;
}
