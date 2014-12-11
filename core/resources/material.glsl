struct Material {
#ifdef MATERIAL_EMISSION
	vec4 emission;
#endif

#ifdef MATERIAL_AMBIENT
	vec4 ambient;
#endif 

#ifdef MATERIAL_DIFFUSE
	vec4 diffuse;
#endif

#ifdef MATERIAL_SPECULAR
	vec4 specular;
	float shininess;
#endif
};
uniform Material u_material;
