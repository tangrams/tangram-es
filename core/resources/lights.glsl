// ------------- These needs to be dinamically injected
// #define NUM_DIRECTIONAL_LIGHTS 1
#define NUM_POINT_LIGHTS 1
// #define NUM_SPOT_LIGHTS 1
// ------------- 

uniform struct Material {
	vec4 emission;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
} u_material;

struct DirectionalLight {
    vec4 ambient;
	vec4 diffuse;
	vec4 specular;

    vec3 direction;
};

void calculateDirectionalLight(in DirectionalLight _light, in vec3 _normal, inout vec4 _ambient, inout vec4 _diffuse, inout vec4 _specular){
    float nDotVP    =  max(0.0,dot(_normal,normalize(vec3(_light.direction))));

    _ambient    += _light.ambient;
    _diffuse    += _light.diffuse * nDotVP;
    _specular   += _light.specular * nDotVP; // We can compute specular better width an extra half vector;
}

struct PointLight {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
   	vec4 position;

    float constantAttenuation;
    float linearAttenuation;
    // float quadraticAttenuation;
};

void calculatePointLight(in PointLight _light, in vec3 _eye, in vec3 _ecPosition3, in vec3 _normal, inout vec4 _ambient, inout vec4 _diffuse, inout vec4 _specular){
    float nDotVP    = 0.0;          // normal . light direction
    float nDotHV    = 0.0;          // normal . light half vector
    float pf        = 0.0;          // power factor
    float attenuation = 1.0;        // computed attenuation factor
    float d         = 0.0;          // distance from surface to light source
    vec3  VP        = vec3(0.0);    // direction from surface to light position
    vec3  halfVector = vec3(0.0);   // direction of maximum highlights

    // Compute vector from surface to light position
    VP = vec3(_light.position) - _ecPosition3;

    // Compute distance between surface and light position
    // d = length(VP);

    // Normalize the vector from surface to light position
    VP = normalize(VP);

    // Compute attenuation
    attenuation = 1.0 / (1.0 + _light.constantAttenuation + _light.linearAttenuation * d);

    halfVector = normalize(VP + _eye);

    nDotVP = max(0.0, dot(_normal, VP));
    nDotHV = max(0.0, dot(_normal, halfVector));

    if (nDotVP == 0.0)
        pf = 0.0;
    else
        pf = pow(nDotHV, u_material.shininess);

    _ambient += _light.ambient * attenuation;
    _diffuse += _light.diffuse * nDotVP * attenuation;
    _specular += _light.specular * pf * attenuation;
}

struct SpotLight {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
   	vec4 position;

   	vec3 direction;

   	float spotExponent;
    float spotCutoff;
    float spotCosCutoff;

    float constantAttenuation;
    // float linearAttenuation;
    // float quadraticAttenuation;
};

void calculateSpotLight(in SpotLight _light, in vec3 _eye, in vec3 _ecPosition3, in vec3 _normal, inout vec4 _ambient, inout vec4 _diffuse, inout vec4 _specular){
    float nDotVP    = 0.0;              // normal . light direction
    float nDotHV    = 0.0;              // normal . light half vector
    float pf        = 0.0;              // power factor
    float spotDot   = 0.0;              // cosine of angle between spotlight
    float spotAttenuation   = 0.0;      // spotlight attenuation factor
    float attenuation       = 0.0;      // computed attenuation factor
    // float d;                         // distance from surface to light source
    vec3 VP         = vec3(0.0);        // direction from surface to light position
    vec3 halfVector = vec3(0.0);        // direction of maximum highlights

    // Compute vector from surface to light position
    VP = vec3(_light.position) - _ecPosition3;

    // Compute distance between surface and light position
    // d = length(VP);

    // Normalize the vector from surface to light position
    VP = normalize(VP);

    // Compute attenuation
    attenuation = 1.0 / (_light.constantAttenuation);// +
                         // _light.linearAttenuation * d +
                         // _light.quadraticAttenuation * d * d);

    // See if point on surface is inside cone of illumination
    spotDot = dot(-VP, normalize(_light.direction));

    if (spotDot < _light.spotCosCutoff)
        spotAttenuation = 0.0; // light adds no contribution
    else
        spotAttenuation = pow(spotDot, _light.spotExponent);

    // Combine the spotlight and distance attenuation.
    attenuation *= spotAttenuation;

    halfVector = normalize(VP + _eye);

    nDotVP = max(0.0, dot(_normal, VP));
    nDotHV = max(0.0, dot(_normal, halfVector));

    if (nDotVP == 0.0)
        pf = 0.0;
    else
        pf = pow(nDotHV, u_material.shininess);

    _ambient  += _light.ambient * attenuation;
    _diffuse  += _light.diffuse * nDotVP * attenuation;
    _specular += _light.specular * pf * attenuation;
}

#ifdef NUM_DIRECTIONAL_LIGHTS
uniform DirectionalLight u_directionalLights[NUM_DIRECTIONAL_LIGHTS];
#endif

#ifdef NUM_POINT_LIGHTS
uniform PointLight u_pointLights[NUM_POINT_LIGHTS];
#endif

#ifdef NUM_SPOT_LIGHTS
uniform SpotLight u_spotLights[NUM_SPOT_LIGHTS];
#endif

vec4 calculateLighting(in vec3 _ecPosition, in vec3 _normal) {
	vec3 eye = vec3(0.0, 0.0, 1.0);
  	// eye = -normalize(_ecPosition3);

  	// Light intensity accumulators
  	vec4 amb  = vec4(0.0);
  	vec4 diff = vec4(0.0);
  	vec4 spec = vec4(0.0);

//	COMPUTE DIRECTIONAL LIGHTS
//
#pragma tangram: DIRECTIONAL_LIGHTS
#ifdef NUM_DIRECTIONAL_LIGHTS
    calculateDirectionalLight(u_directionalLights[0], _normal, amb, diff, spec);
#endif

//	COMPUTE POINT LIGHTS
//
#pragma tangram: POINT_LIGHTS
#ifdef NUM_POINT_LIGHTS
    calculatePointLight(u_pointLights[0], eye, _ecPosition, _normal, amb, diff, spec);
#endif

//	COMPUTE SPOT LIGHTS
//
#pragma tangram: SPOT_LIGHTS
#ifdef NUM_SPOT_LIGHTS
    calculateSpotLight(u_spotLights[0], eye, _ecPosition, _normal, amb, diff, spec);
#endif

//  Final light intensity calculation
//
    return  amb * u_material.ambient + diff * u_material.diffuse + spec * u_material.specular;
}