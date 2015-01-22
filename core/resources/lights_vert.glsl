
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

void calculateLighting(in vec3 _eyeToPoint, in vec3 _normal, inout vec4 _colorOut) {
    
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

        color = clamp(color, 0.0, 1.0);
        _colorOut *= color;

    #endif
}
