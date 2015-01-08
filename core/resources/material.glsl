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

// Note: uniforms (u_material and u_ligth) are copy to global instances (g_material and g_light) to let the user modify them in the shader
