struct SpotLight {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 position;

    vec3 direction;

    float spotExponent;
    float spotCutoff;
    float spotCosCutoff;
};

void calculateLight(in SpotLight _light, in vec3 _eye, in vec3 _ecPosition3, in vec3 _normal, inout vec4 _ambient, inout vec4 _diffuse, inout vec4 _specular){
    // Compute vector from surface to light position
    vec3 VP = vec3(_light.position) - _ecPosition3;

    // Normalize the vector from surface to light position
    VP = normalize(VP);
    
    // spotlight attenuation factor
    float spotAttenuation = 0.0;
    if (spotDot >= _light.spotCosCutoff){

        // See if point on surface is inside cone of illumination
        float spotDot = dot(-VP, normalize(_light.direction));
        spotAttenuation = pow(spotDot, _light.spotExponent);
    }
        
    // normal . light direction
    float nDotVP = max(0.0, dot(_normal, VP));

#ifdef MATERIAL_AMBIENT
    _ambient  += _light.ambient * spotAttenuation;
#endif

#ifdef MATERIAL_DIFFUSE 
    _diffuse  += _light.diffuse * nDotVP * spotAttenuation;
#endif

#ifdef MATERIAL_SPECULAR
    
    // Power factor for shinny speculars
    float pf = 0.0;              
    if (nDotVP != 0.0){
        // Direction of maximum highlights
        vec3 halfVector = normalize(VP + _eye);

        // normal . light half vector
        float nDotHV = max(0.0, dot(_normal, halfVector));
        pf = pow(nDotHV, u_material.shininess);
    }
        
    _specular += _light.specular * pf * spotAttenuation;
#endif

}
