
// TODO: 
//		- BM - port this to STRINGIFY on C++

#ifdef TANGRAM_FRAGMENT_LIGHTS
	#ifdef TANGRAM_MATERIAL_AMBIENT
varying vec4 v_light_accumulator_ambient;
	#endif
	#ifdef TANGRAM_MATERIAL_DIFFUSE
varying vec4 v_light_accumulator_diffuse;
	#endif
	#ifdef TANGRAM_MATERIAL_SPECULAR
varying vec4 v_light_accumulator_specular;
	#endif
#endif

#ifdef TANGRAM_FRAGMENT_LIGHTS
void calculateLighting(in vec3 _eyeToPoint, in vec3 _normal ) {
#else
vec4 calculateLighting(in vec3 _eyeToPoint, in vec3 _normal ) {
#endif

	vec3 eye = vec3(0.0, 0.0, 1.0);

	#ifdef TANGRAM_MATERIAL_AMBIENT
	g_light_accumulator_ambient	= vec4(0.0);
	#endif

	#ifdef TANGRAM_MATERIAL_DIFFUSE
	g_light_accumulator_diffuse = vec4(0.0);
	#endif

	#ifdef TANGRAM_MATERIAL_SPECULAR
	g_light_accumulator_specular = vec4(0.0);
	#endif

#pragma tangram: vertex_lights_to_compute

#ifdef TANGRAM_FRAGMENT_LIGHTS

	#ifdef TANGRAM_MATERIAL_AMBIENT
	v_light_accumulator_ambient = g_light_accumulator_ambient;
	#endif

	#ifdef TANGRAM_MATERIAL_DIFFUSE
	v_light_accumulator_diffuse = g_light_accumulator_diffuse;
	#endif

	#ifdef TANGRAM_MATERIAL_SPECULAR
	v_light_accumulator_specular = g_light_accumulator_specular;
	#endif

#else 
	vec4 color = vec4(0.0);
  
	#ifdef TANGRAM_MATERIAL_EMISSION
	color = g_material.emission;
	#endif

	#ifdef TANGRAM_MATERIAL_AMBIENT
	color += g_light_accumulator_ambient * g_material.ambient;
	#endif

	#ifdef TANGRAM_MATERIAL_DIFFUSE
	color += g_light_accumulator_diffuse * g_material.diffuse;
	#endif

	#ifdef TANGRAM_MATERIAL_SPECULAR
	color += g_light_accumulator_specular * g_material.specular;
	#endif

	color.r = clamp(0.0,1.0,color.r);
	color.g = clamp(0.0,1.0,color.g);
	color.b = clamp(0.0,1.0,color.b);
	color.a = clamp(0.0,1.0,color.a);
	
	return color;
#endif
}
