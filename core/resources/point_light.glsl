struct PointLight {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 position;

	#ifdef TANGRAM_POINTLIGHT_CONSTANT_ATTENUATION
	#define TANGRAM_POINTLIGHT_ATTENUATION
	float constantAttenuation;
	#endif

	#ifdef TANGRAM_POINTLIGHT_LINEAR_ATTENUATION
	#ifndef TANGRAM_POINTLIGHT_ATTENUATION
	#define TANGRAM_POINTLIGHT_ATTENUATION
	#endif
	#define TANGRAM_POINTLIGHT_DISTANCE
	float linearAttenuation;
	#endif


	#ifdef TANGRAM_POINTLIGHT_QUADRATIC_ATTENUATION
	#ifndef TANGRAM_POINTLIGHT_ATTENUATION
	#define TANGRAM_POINTLIGHT_ATTENUATION
	#endif
	#ifndef TANGRAM_POINTLIGHT_DISTANCE
	#define TANGRAM_POINTLIGHT_DISTANCE
	#endif
	float quadraticAttenuation;
	#endif
};

void calculateLight(in PointLight _light, in vec3 _eye, in vec3 _eyeToPoint, in vec3 _normal){

	// Compute vector from surface to light position
	vec3 VP = normalize( vec3(_light.position) - _eyeToPoint );

	#ifdef TANGRAM_POINTLIGHT_DISTANCE
	float dist = length( vec3(_light.position) - _eyeToPoint );
	#endif 

	// Normalize the vector from surface to light position
	float nDotVP = min( max(0.0, dot(VP, _normal) ),1.0);

	#ifdef TANGRAM_POINTLIGHT_ATTENUATION
	float atFactor = 0.0;

	#ifdef TANGRAM_POINTLIGHT_CONSTANT_ATTENUATION
	atFactor += _light.constantAttenuation;
	#endif

	#ifdef TANGRAM_POINTLIGHT_LINEAR_ATTENUATION
	atFactor += _light.linearAttenuation * dist;
	#endif

	#ifdef TANGRAM_POINTLIGHT_QUADRATIC_ATTENUATION
	atFactor += _light.quadraticAttenuation * dist * dist;
	#endif
	
	float attenuation = 1.0;
	if(atFactor!=0.0){
		attenuation /= atFactor;
	}
	#endif

	#ifdef TANGRAM_MATERIAL_AMBIENT
	#ifdef TANGRAM_POINTLIGHT_ATTENUATION
	g_light_accumulator_ambient += _light.ambient * attenuation;
	#else
	g_light_accumulator_ambient += _light.ambient;
	#endif
	#endif
	
	#ifdef TANGRAM_MATERIAL_DIFFUSE 
	#ifdef TANGRAM_POINTLIGHT_ATTENUATION
	g_light_accumulator_diffuse += _light.diffuse * nDotVP * attenuation;
	#else
	g_light_accumulator_diffuse += _light.diffuse * nDotVP;
	#endif
	#endif

	#ifdef TANGRAM_MATERIAL_SPECULAR
	float pf = 0.0; // power factor for shinny speculars
	if (nDotVP > 0.0){
		vec3 halfVector = normalize(VP + _eye); // Direction of maximum highlights
		float nDotHV = max(0.0, dot(_normal, halfVector) );
		pf = pow(nDotHV, g_material.shininess);
	}

	#ifdef TANGRAM_POINTLIGHT_ATTENUATION
	g_light_accumulator_specular += _light.specular * pf * attenuation;
	#else
	g_light_accumulator_specular += _light.specular * pf;
	#endif

	#endif
}
