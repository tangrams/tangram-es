#ifdef GL_ES
precision mediump float;
#endif

//---------------	lights.glsl
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

void DirectionalLight(in int _i, in vec3 _normal,inout vec4 _ambient, inout vec4 _diffuse, inout vec4 _specular){
     float nDotVP;         // normal . light direction
     float nDotHV;         // normal . light half vector
     float pf;             // power factor

     nDotVP = max(0.0, dot(_normal, normalize(vec3(u_lights[_i].position))));
     nDotHV = max(0.0, dot(_normal, vec3(u_lights[_i].halfVector)));

     if (nDotVP == 0.0)
         pf = 0.0;
     else
         pf = pow(nDotHV, u_material.shininess);

     _ambient  += u_lights[_i].ambient;
     _diffuse  += u_lights[_i].diffuse * nDotVP;
     _specular += u_lights[_i].specular * pf;
}

void PointLight(in int _i, in vec3 _eye, in vec3 _ecPosition3, in vec3 _normal, inout vec4 _ambient, inout vec4 _diffuse, inout vec4 _specular){
    float nDotVP;         // normal . light direction
    float nDotHV;         // normal . light half vector
    float pf;             // power factor
    float attenuation;    // computed attenuation factor
    float d;              // distance from surface to light source
    vec3  VP;             // direction from surface to light position
    vec3  halfVector;     // direction of maximum highlights

    // Compute vector from surface to light position
    VP = vec3(u_lights[_i].position) - _ecPosition3;

    // Compute distance between surface and light position
    d = length(VP);

    // Normalize the vector from surface to light position
    VP = normalize(VP);

    // Compute attenuation
    attenuation = 1.0 / (u_lights[_i].constantAttenuation +
                         u_lights[_i].linearAttenuation * d +
                         u_lights[_i].quadraticAttenuation * d * d);

    halfVector = normalize(VP + _eye);

    nDotVP = max(0.0, dot(_normal, VP));
    nDotHV = max(0.0, dot(_normal, halfVector));

    if (nDotVP == 0.0)
        pf = 0.0;
    else
        pf = pow(nDotHV, u_material.shininess);

    _ambient += u_lights[_i].ambient * attenuation;
    _diffuse += u_lights[_i].diffuse * nDotVP * attenuation;
    _specular += u_lights[_i].specular * pf * attenuation;
}           

void SpotLight(in int _i, in vec3 _eye, vec3 _ecPosition3, in vec3 _normal, inout vec4 _ambient, inout vec4 _diffuse, inout vec4 _specular){
    float nDotVP;           // normal . light direction
    float nDotHV;           // normal . light half vector
    float pf;               // power factor
    float spotDot;          // cosine of angle between spotlight
    float spotAttenuation;  // spotlight attenuation factor
    float attenuation;      // computed attenuation factor
    float d;                // distance from surface to light source
    vec3 VP;                // direction from surface to light position
    vec3 halfVector;        // direction of maximum highlights

    // Compute vector from surface to light position
    VP = vec3(u_lights[_i].position) - _ecPosition3;

    // Compute distance between surface and light position
    d = length(VP);

    // Normalize the vector from surface to light position
    VP = normalize(VP);

    // Compute attenuation
    attenuation = 1.0 / (u_lights[_i].constantAttenuation +
                         u_lights[_i].linearAttenuation * d +
                         u_lights[_i].quadraticAttenuation * d * d);

    // See if point on surface is inside cone of illumination
    spotDot = dot(-VP, normalize(u_lights[_i].direction));

    if (spotDot < u_lights[_i].spotCosCutoff)
        spotAttenuation = 0.0; // light adds no contribution
    else
        spotAttenuation = pow(spotDot, u_lights[_i].spotExponent);

    // Combine the spotlight and distance attenuation.
    attenuation *= spotAttenuation;

    halfVector = normalize(VP + _eye);

    nDotVP = max(0.0, dot(_normal, VP));
    nDotHV = max(0.0, dot(_normal, halfVector));

    if (nDotVP == 0.0)
        pf = 0.0;
    else
        pf = pow(nDotHV, u_material.shininess);

    _ambient  += u_lights[_i].ambient * attenuation;
    _diffuse  += u_lights[_i].diffuse * nDotVP * attenuation;
    _specular += u_lights[_i].specular * pf * attenuation;
} 

vec4 calculateLighting(in vec3 _ecPosition, in vec3 _normal) {
	vec3 eye = vec3(0.0, 0.0, 1.0);
  	//eye = -normalize(ecPosition3);

  	// Clear the light intensity accumulators
  	vec4 amb  = vec4(0.0);
  	vec4 diff = vec4(0.0);
  	vec4 spec = vec4(0.0);

  	// Loop through enabled lights, compute contribution from each
  	for (int i = 0; i < NUM_LIGHTS; i++){
    	if (u_lights[i].position.w == 0.0)
        	DirectionalLight(i, normalize(_normal), amb, diff, spec);
      	else if (u_lights[i].spotCutoff == 180.0)
        	PointLight(i, eye, _ecPosition, normalize(_normal), amb, diff, spec);
      	else
        	SpotLight(i, eye, _ecPosition, normalize(_normal), amb, diff, spec);
  	}

	vec4 color =  	amb * u_material.ambient + 
#ifdef COLOR_TEXTURE
                	diff * texture2D(u_textureDiffuse, a_uv) +
#else
                	diff * u_material.diffuse +
#endif
                	spec * u_material.specular;

  return color;
}
//---------- light.glsl

varying vec4 v_pos;
varying vec4 v_color;
varying vec3 v_normal;
varying vec2 v_uv;

void main(void) {
	vec4 color = v_color;
  	gl_FragColor = color;
}
