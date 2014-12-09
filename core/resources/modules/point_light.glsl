struct PointLight {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
   	vec4 position;
};

void calculateLight(in PointLight _light, in vec3 _eye, in vec3 _ecPosition3, in vec3 _normal, inout vec4 _ambient, inout vec4 _diffuse, inout vec4 _specular){

#ifdef MATERIAL_AMBIENT
    _ambient += _light.ambient;
#endif

    // Compute vector from surface to light position
    vec3 VP = vec3(_light.position) - _ecPosition3;

    // Normalize the vector from surface to light position
    VP = normalize(VP);
    float nDotVP = min(max(0.0, dot(VP,_normal)),1.0);
    
#ifdef MATERIAL_DIFFUSE 
    _diffuse += _light.diffuse * nDotVP;
#endif

#ifdef MATERIAL_SPECULAR
    float pf = 0.0; // power factor for shinny speculars
    if (nDotVP > 0.0){
        vec3 halfVector = normalize(VP + _eye); // Direction of maximum highlights
        float nDotHV = max(0.0, dot(_normal, halfVector));
        pf = pow(nDotHV, u_material.shininess);
    }
    _specular += _light.specular * pf;
#endif
}