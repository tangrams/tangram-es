varying vec4 v_amb;
varying vec4 v_diff;
varying vec4 v_spec;

#define TANGRAM_FRAGMENT_LIGHTS
vec4 calculateLighting(in vec3 _eyeToPoint, in vec3 _normal) {
	vec3 eye = vec3(0.0, 0.0, 1.0);

	vec4 amb = v_amb;
	vec4 diff = v_diff;
	vec4 spec = v_spec;

#pragma tangram: fragment_lights_to_compute

	//  Final light intensity calculation
	//
	vec4 color = vec4(0.0);
  
	#ifdef TANGRAM_MATERIAL_EMISSION
	color = g_material.emission;
	#endif

	#ifdef TANGRAM_MATERIAL_AMBIENT
	color += amb * g_material.ambient;
	#endif

	#ifdef TANGRAM_MATERIAL_DIFFUSE
	color += diff * g_material.diffuse;
	#endif

	#ifdef TANGRAM_MATERIAL_SPECULAR
	color += spec * g_material.specular;
	#endif

	//  For the moment no alpha light (weird concept... right?)
	color.a = 1.0;

	return color;
}
