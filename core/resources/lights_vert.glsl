varying vec4 v_amb;
varying vec4 v_diff;
varying vec4 v_spec;

void calculateLighting(in vec3 _eyeToPoint, in vec3 _normal) {
	vec3 eye = vec3(0.0, 0.0, 1.0);

	// Light intensity accumulators
	vec4 amb  = vec4(0.0);
	vec4 diff = vec4(0.0);
	vec4 spec = vec4(0.0);

	v_amb	= vec4(0.0);
	v_diff 	= vec4(0.0);
	v_spec 	= vec4(0.0);

//  COMPUTE ALL LIGHTS
#pragma tangram: vert_lights_to_compute

	#ifdef MATERIAL_AMBIENT
	v_amb = amb;
	#endif

	#ifdef MATERIAL_DIFFUSE
	v_diff = diff;
	#endif

	#ifdef MATERIAL_SPECULAR
	v_spec = spec;
	#endif
}
