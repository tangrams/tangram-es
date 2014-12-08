vec4 calculateLighting(in vec3 _ecPosition, in vec3 _normal) {
	vec3 eye = vec3(0.0, 0.0, 1.0);
  	// eye = -normalize(_ecPosition3);

  	// Light intensity accumulators
  	vec4 amb  = vec4(0.0);
  	vec4 diff = vec4(0.0);
  	vec4 spec = vec4(0.0);

//  COMPUTE DIRECTIONAL LIGHTS
#pragma tangram: directional_lights

//  COMPUTE POINT LIGHTS
#pragma tangram: point_lights

//  COMPUTE SPOT LIGHTS
#pragma tangram: spot_ligths

//  TODO: - implement array of materials
//
#ifdef GENERAL_MATERIAL
    amb *= u_material.ambient;
    diff *= u_material.diffuse;
    spec *= u_material.specular;
#endif

  //  Final light intensity calculation
  //
  return  amb + diff + spec ;
}
