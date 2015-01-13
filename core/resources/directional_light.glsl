struct DirectionalLight {
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;

	vec3 direction;
};

void calculateLight(in DirectionalLight _light, in vec3 _eye, in vec3 _eyeToPoint, in vec3 _normal){
	#ifdef TANGRAM_MATERIAL_AMBIENT
	g_light_accumulator_ambient += _light.ambient;
	#endif

	float nDotVP =  min( max(0.0, dot(_normal, normalize( vec3(_light.direction) ) ) ), 1.0);

	#ifdef TANGRAM_MATERIAL_DIFFUSE	
	g_light_accumulator_diffuse += _light.diffuse * nDotVP;
	#endif

	#ifdef TANGRAM_MATERIAL_SPECULAR
	float pf = 0.0;
	if (nDotVP != 0.0){
		vec3 halfVector = normalize( vec3(_light.direction) + _eye );
		float nDotHV = min( max(0.0, dot(_normal, halfVector) ), 1.0 );
		pf = pow(nDotHV, g_material.shininess);
	}
	g_light_accumulator_specular += _light.specular * pf;
	#endif
}
