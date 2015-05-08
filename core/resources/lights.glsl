vec4 calculateLighting(in vec3 _eyeToPoint, in vec3 _normal, in vec4 _color) {

    // Do initial material calculations over normal, emission, ambient, diffuse and specular values
    calculateMaterial(_eyeToPoint,_normal);
   

    // Un roll the loop of individual ligths to calculate
    #pragma tangram: lights_to_compute

    //  Final light intensity calculation
    vec4 color = vec4(0.0);

    #ifdef TANGRAM_MATERIAL_EMISSION
        color = material.emission;
    #endif

    #ifdef TANGRAM_MATERIAL_AMBIENT
        color += light_accumulator_ambient * _color * material.ambient;
    #else
        #ifdef TANGRAM_MATERIAL_DIFFUSE
            color += light_accumulator_ambient * _color * material.diffuse;
        #endif
    #endif

    #ifdef TANGRAM_MATERIAL_DIFFUSE
        color += light_accumulator_diffuse * _color * material.diffuse;
    #endif

    #ifdef TANGRAM_MATERIAL_SPECULAR
        color += light_accumulator_specular * material.specular;
    #endif

    // Clamp final color
    color = clamp(color, 0.0, 1.0);

    return color;
}
