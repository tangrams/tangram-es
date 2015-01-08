#define TANGRAM_VERTEX_LIGHTS

varying vec4 v_amb;
varying vec4 v_diff;
varying vec4 v_spec;

// TODO: 
//		- BM - lights accumulators in a varying struct 
//		- BM - calcLights functions return them in a clear way
//
// 			void calculateLighting(in vec3 _eyeToPoint, in vec3 _normal, out LightAccumulation _accumulation)
//		or
//			LightAccumulation calculateLighting(in vec3 _eyeToPoint, in vec3 _normal)
//
//		- BM - port this to STRINGIFY on C++

void calculateLighting(in vec3 _eyeToPoint, in vec3 _normal) {
	vec3 eye = vec3(0.0, 0.0, 1.0);

	// Light intensity accumulators
	vec4 amb  = vec4(0.0);
	vec4 diff = vec4(0.0);
	vec4 spec = vec4(0.0);

	v_amb	= vec4(0.0);
	v_diff 	= vec4(0.0);
	v_spec 	= vec4(0.0);

#pragma tangram: vertex_lights_to_compute

	#ifdef TANGRAM_MATERIAL_AMBIENT
	v_amb = amb;
	#endif

	#ifdef TANGRAM_MATERIAL_DIFFUSE
	v_diff = diff;
	#endif

	#ifdef TANGRAM_MATERIAL_SPECULAR
	v_spec = spec;
	#endif

// TODO :
//	- BM - Clamp accumulators results to 1.

}
