vec4 calculateLighting(in vec3 _ecPosition, in vec3 _normal) {
	vec3 eye = vec3(0.0, 0.0, 1.0);

  // Light intensity accumulators
  vec4 amb  = vec4(0.0);
  vec4 diff = vec4(0.0);
  vec4 spec = vec4(0.0);

//  COMPUTE ALL LIGHTS
#pragma tangram: lights_unrol_loop

  //  Final light intensity calculation
  //
  vec4 color = vec4(0.0);
  
#ifdef MATERIAL_EMISSION
  color = u_material.emission;
#endif

#ifdef MATERIAL_AMBIENT
  color += amb * u_material.ambient;
#endif

#ifdef MATERIAL_DIFFUSE
    color += diff * u_material.diffuse;
#endif

#ifdef MATERIAL_SPECULAR
    color += spec * u_material.specular;
#endif

  return color;
}
