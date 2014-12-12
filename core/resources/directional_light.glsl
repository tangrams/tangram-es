struct DirectionalLight {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;

    vec3 direction;
};

void calculateLight(in DirectionalLight _light, in vec3 _eye, in vec3 _ecPosition3, in vec3 _normal, inout vec4 _ambient, inout vec4 _diffuse, inout vec4 _specular){
#ifdef MATERIAL_AMBIENT
    _ambient    += _light.ambient;
#endif

float nDotVP =  max(0.0,dot(_normal,normalize(vec3(_light.direction))));

#ifdef MATERIAL_DIFFUSE	
    _diffuse    += _light.diffuse * nDotVP;
#endif

#ifdef MATERIAL_SPECULAR
    float pf = 0.0;
    if (nDotVP != 0.0){
    	vec3 halfVector = normalize(vec3(_light.direction) + _eye);
		float nDotHV = min(max(0.0, dot(_normal, halfVector)),1.0);
    	pf = pow(nDotHV, g_material.shininess);
    }
    _specular   += _light.specular * pf;
#endif
}
