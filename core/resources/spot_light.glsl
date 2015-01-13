struct SpotLight {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;

    vec4 position;
    vec3 direction;

    float spotCosCutoff;
    float spotExponent;

    #ifdef TANGRAM_SPOTLIGHT_CONSTANT_ATTENUATION
    #define TANGRAM_SPOTLIGHT_ATTENUATION
    float constantAttenuation;
    #endif

    #ifdef TANGRAM_SPOTLIGHT_LINEAR_ATTENUATION
    #ifndef TANGRAM_SPOTLIGHT_ATTENUATION
        #define TANGRAM_SPOTLIGHT_ATTENUATION
    #endif
    #define TANGRAM_SPOTLIGHT_DISTANCE
    float linearAttenuation;
    #endif

    #ifdef TANGRAM_SPOTLIGHT_QUADRATIC_ATTENUATION
    #ifndef TANGRAM_SPOTLIGHT_ATTENUATION
        #define TANGRAM_SPOTLIGHT_ATTENUATION
    #endif
    #ifndef TANGRAM_SPOTLIGHT_DISTANCE
        #define TANGRAM_SPOTLIGHT_DISTANCE
    #endif
    float quadraticAttenuation;
    #endif
};

void calculateLight(in SpotLight _light, in vec3 _eye, in vec3 _eyeToPoint, in vec3 _normal){
    // Compute vector from surface to light position
    vec3 VP = normalize( vec3(_light.position) - _eyeToPoint );

    #ifdef TANGRAM_SPOTLIGHT_DISTANCE
    float dist = length( vec3(_light.position) - _eyeToPoint );
    #endif 

    // spotlight attenuation factor
    float spotAttenuation = 0.0;

    // See if point on surface is inside cone of illumination
    float spotDot = min( max(0.0, dot(-VP, normalize( _light.direction ) ) ), 1.0);
    if (spotDot < _light.spotCosCutoff){
        spotAttenuation = 0.0;
    } else {
        spotAttenuation = pow( spotDot, _light.spotExponent );
    }

    #ifdef TANGRAM_SPOTLIGHT_ATTENUATION
    float atFactor = 0.0;
    {
        #ifdef TANGRAM_SPOTLIGHT_CONSTANT_ATTENUATION
        atFactor += _light.constantAttenuation;
        #endif

        #ifdef TANGRAM_SPOTLIGHT_LINEAR_ATTENUATION
        atFactor += _light.linearAttenuation * dist;
        #endif
            
        #ifdef TANGRAM_SPOTLIGHT_QUADRATIC_ATTENUATION
        atFactor += _light.quadraticAttenuation * dist * dist;
        #endif
    }
    spotAttenuation *= 1.0 /atFactor;
    #endif

    // normal . light direction
    float nDotVP = min( max(0.0, dot( _normal, VP ) ), 1.0);

    #ifdef TANGRAM_MATERIAL_AMBIENT
    g_light_accumulator_ambient  += _light.ambient * spotAttenuation;
    #endif

    #ifdef TANGRAM_MATERIAL_DIFFUSE 
    g_light_accumulator_diffuse  += _light.diffuse * nDotVP * spotAttenuation;
    #endif

    #ifdef TANGRAM_MATERIAL_SPECULAR
    // Power factor for shinny speculars
    float pf = 0.0;              
    if (nDotVP != 0.0){
        // Direction of maximum highlights
        vec3 halfVector = normalize(VP + _eye);

        // normal . light half vector
        float nDotHV = min( max(0.0, dot( _normal, halfVector ) ),1.0);
        pf = pow(nDotHV, g_material.shininess);
    }
    g_light_accumulator_specular += _light.specular * pf * spotAttenuation;
    #endif
}
