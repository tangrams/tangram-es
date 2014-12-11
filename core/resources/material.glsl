//	TODO:
//			- Material should be injected as global
//

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

//
//	BRAIN-STORIMING Posible???
//
//uniform struct EnvMaterial {
// #ifdef MATERIAL_EMISSION_MAP
// 	sampler2D emissionMap;
// #endif
//
// #ifdef MATERIAL_AMBIENT_MAP
// 	sampler2D ambientMap;
// #endif
//
// #ifdef MATERIAL_DIFFUSE_MAP
// 	sampler2D diffuseMap;
// #endif
//
// #ifdef MATERIAL_SPECULAR_MAP
// 	sampler2D spedcularMap;
// #endif
//} u_material_env;