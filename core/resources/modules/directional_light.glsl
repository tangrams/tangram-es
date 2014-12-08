struct DirectionalLight {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;

    vec3 direction;
};

void calculateDirectionalLight(in DirectionalLight _light, in vec3 _normal, inout vec4 _ambient, inout vec4 _diffuse, inout vec4 _specular){
    float nDotVP =  max(0.0,dot(_normal,normalize(vec3(_light.direction))));

    _ambient    += _light.ambient;
    _diffuse    += _light.diffuse * nDotVP;
    _specular   += _light.specular * nDotVP;
}
