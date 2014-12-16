struct PointLight {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 position;

	#ifdef POINTLIGHT_CONSTANT_ATTENUATION
	#define POINTLIGHT_ATTENUATION
	float constantAttenuation;
	#endif

	#ifdef POINTLIGHT_LINEAR_ATTENUATION
	#ifndef POINTLIGHT_ATTENUATION
	#define POINTLIGHT_ATTENUATION
	#endif
	#define POINTLIGHT_DISTANCE
	float linearAttenuation;
	#endif


	#ifdef POINTLIGHT_QUADRATIC_ATTENUATION
	#ifndef POINTLIGHT_ATTENUATION
	#define POINTLIGHT_ATTENUATION
	#endif
	#ifndef POINTLIGHT_DISTANCE
	#define POINTLIGHT_DISTANCE
	#endif
	float quadraticAttenuation;
	#endif
};

void calculateLight(in PointLight _light, in vec3 _eye, in vec3 _eyeToPoint, in vec3 _normal, inout vec4 _ambient, inout vec4 _diffuse, inout vec4 _specular){

	// Compute vector from surface to light position
	vec3 VP = normalize( vec3(_light.position) - _eyeToPoint );

	#ifdef POINTLIGHT_DISTANCE
	float dist = length( vec3(_light.position) - _eyeToPoint );
	#endif 

	// Normalize the vector from surface to light position
	float nDotVP = min( max(0.0, dot(VP, _normal) ),1.0);

	#ifdef POINTLIGHT_ATTENUATION
	float atFactor = 0.0;

	#ifdef POINTLIGHT_CONSTANT_ATTENUATION
	atFactor += _light.constantAttenuation;
	#endif

	#ifdef POINTLIGHT_LINEAR_ATTENUATION
	atFactor += _light.linearAttenuation * dist;
	#endif

	#ifdef POINTLIGHT_QUADRATIC_ATTENUATION
	atFactor += _light.quadraticAttenuation * dist * dist;
	#endif
	
	float attenuation = 1.0;
	if(atFactor!=0.0){
		attenuation /= atFactor;
	}
	#endif

	#ifdef MATERIAL_AMBIENT
	#ifdef POINTLIGHT_ATTENUATION
	_ambient += _light.ambient * attenuation;
	#else
	_ambient += _light.ambient;
	#endif
	#endif
	
	#ifdef MATERIAL_DIFFUSE 
	#ifdef POINTLIGHT_ATTENUATION
	_diffuse += _light.diffuse * nDotVP * attenuation;
	#else
	_diffuse += _light.diffuse * nDotVP;
	#endif
	#endif

	#ifdef MATERIAL_SPECULAR
	float pf = 0.0; // power factor for shinny speculars
	if (nDotVP > 0.0){
		vec3 halfVector = normalize(VP + _eye); // Direction of maximum highlights
		float nDotHV = max(0.0, dot(_normal, halfVector) );
		pf = pow(nDotHV, g_material.shininess);
	}

	#ifdef POINTLIGHT_ATTENUATION
	_specular += _light.specular * pf * attenuation;
	#else
	_specular += _light.specular * pf;
	#endif

	#endif
}
