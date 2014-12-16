vec4 calculateLighting(in vec3 _eyeToPoint, in vec3 _normal) {
	vec3 eye = vec3(0.0, 0.0, 1.0);

  // Light intensity accumulators
  vec4 amb  = vec4(0.0);
  vec4 diff = vec4(0.0);
  vec4 spec = vec4(0.0);

//  COMPUTE ALL LIGHTS
#pragma tangram: lights_to_compute

  //  Final light intensity calculation
  //
  vec4 color = vec4(0.0);
  
#ifdef MATERIAL_EMISSION
  color = g_material.emission;
#endif

#ifdef MATERIAL_AMBIENT
  color += amb * g_material.ambient;
#endif

#ifdef MATERIAL_DIFFUSE
    color += diff * g_material.diffuse;
#endif

#ifdef MATERIAL_SPECULAR
    color += spec * g_material.specular;
#endif

  //  For the moment no alpha light (weird concept... right?)
  color.a = 1.0;

  return color;
}
