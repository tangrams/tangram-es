struct Material {
	vec4 emission;
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float shininess;
};

#define GENERAL_MATERIAL
uniform Material u_material;
