struct Material {
	#ifdef TANGRAM_MATERIAL_EMISSION
	vec4 emission;
	#endif

	#ifdef TANGRAM_MATERIAL_AMBIENT
	vec4 ambient;
	#endif 

	#ifdef TANGRAM_MATERIAL_DIFFUSE
	vec4 diffuse;
	#endif

	#ifdef TANGRAM_MATERIAL_SPECULAR
	vec4 specular;
	float shininess;
	#endif
};

uniform Material u_material;
Material g_material = u_material;
