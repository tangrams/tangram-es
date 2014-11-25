// #define NUM_DIRECTIONAL_LIGHTS 1
// #define NUM_POINT_LIGHTS 1
#define NUM_SPOT_LIGHTS 1

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
    // vec3 halfVector;
};

void calculateDirectionalLight(in DirectionalLight _light, in vec3 _normal, inout vec4 _ambient, inout vec4 _diffuse, inout vec4 _specular){
	vec3  halfVector;
    float nDotVP;         // normal . light direction
    float nDotHV;         // normal . light half vector
    float pf;             // power factor

    nDotVP = max(0.0, dot(_normal, normalize(vec3(_light.direction))));
    // nDotHV = max(0.0, dot(_normal, vec3(_light.halfVector)));

    // if (nDotVP == 0.0)
    //     pf = 0.0;
    // else
    //     pf = pow(nDotHV, u_material.shininess);

    _ambient  += _light.ambient;
    _diffuse  += _light.diffuse * nDotVP;
    // _specular += _light.specular * pf;
}

struct PointLight {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
   	vec4 position;

    float constantAttenuation;
    // float linearAttenuation;
    // float quadraticAttenuation;
};

void calculatePointLight(in PointLight _light, in vec3 _eye, in vec3 _ecPosition3, in vec3 _normal, inout vec4 _ambient, inout vec4 _diffuse, inout vec4 _specular){
    float nDotVP;         // normal . light direction
    float nDotHV;         // normal . light half vector
    float pf;             // power factor
    float attenuation;    // computed attenuation factor
    float d;              // distance from surface to light source
    vec3  VP;             // direction from surface to light position
    vec3  halfVector;     // direction of maximum highlights

    // Compute vector from surface to light position
    VP = vec3(_light.position) - _ecPosition3;

    // Compute distance between surface and light position
    d = length(VP);

    // Normalize the vector from surface to light position
    VP = normalize(VP);

    // Compute attenuation
    attenuation = 1.0 / (_light.constantAttenuation);// +
                         // _light.linearAttenuation * d +
                         // _light.quadraticAttenuation * d * d);

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
    VP = vec3(_light.position) - _ecPosition3;

    // Compute distance between surface and light position
    d = length(VP);

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
  	//eye = -normalize(_ecPosition3);

  	// Clear the light intensity accumulators
  	vec4 amb  = vec4(0.0);
  	vec4 diff = vec4(0.0);
  	vec4 spec = vec4(0.0);

//	COMPUTE DIRECTIONAL LIGHTS
//
// #pragma tangram: DIRECTIONAL_LIGHTS
#ifdef NUM_DIRECTIONAL_LIGHTS
    calculateDirectionalLight(u_directionalLights[0], _normal, amb, diff, spec);
#endif

//	COMPUTE POINT LIGHTS
//
// #pragma tangram: POINT_LIGHTS
#ifdef NUM_POINT_LIGHTS
    calculatePointLight(u_pointLights[0], eye, _ecPosition, _normal, amb, diff, spec);
#endif

//	COMPUTE SPOT LIGHTS
//
// #pragma tangram: SPOT_LIGHTS
#ifdef NUM_SPOT_LIGHTS
    calculateSpotLight(u_spotLights[0], eye, _ecPosition, _normal, amb, diff, spec);
#endif

	vec4 color =  	amb * u_material.ambient + 
#ifdef COLOR_TEXTURE
                	diff * texture2D(u_textureDiffuse, a_uv) +
#else
                	diff * u_material.diffuse +
#endif
                	spec * u_material.specular;

  return color;
}