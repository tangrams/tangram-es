
//  TODO BM:
//      - port this to STRINGIFY on C++

#ifdef TANGRAM_VERTEX_LIGHTS
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

void calculateLighting(in vec3 _eyeToPoint, in vec3 _normal, inout vec4 _colorOut) {

    #ifdef TANGRAM_VERTEX_LIGHTS
        #ifdef TANGRAM_MATERIAL_AMBIENT
            g_light_accumulator_ambient = v_light_accumulator_ambient;
        #endif

        #ifdef TANGRAM_MATERIAL_DIFFUSE
            g_light_accumulator_diffuse = v_light_accumulator_diffuse;
        #endif

        #ifdef TANGRAM_MATERIAL_SPECULAR
            g_light_accumulator_specular = v_light_accumulator_specular;
        #endif
    #else
        #ifdef TANGRAM_MATERIAL_AMBIENT
            g_light_accumulator_ambient = vec4(0.0);
        #endif

        #ifdef TANGRAM_MATERIAL_DIFFUSE
            g_light_accumulator_diffuse = vec4(0.0);
        #endif

        #ifdef TANGRAM_MATERIAL_SPECULAR
            g_light_accumulator_specular = vec4(0.0);
        #endif
    #endif

    #pragma tangram: fragment_lights_to_compute

    //  Final light intensity calculation
    //
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

    color.r = clamp(color.r,0.0,1.0);
    color.g = clamp(color.g,0.0,1.0);
    color.b = clamp(color.b,0.0,1.0);
    color.a = clamp(color.a,0.0,1.0);

    _colorOut *= color;
}
