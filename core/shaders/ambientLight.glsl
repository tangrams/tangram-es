/*

Expected globals:
light_accumulator_*

*/

struct AmbientLight {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
};

void calculateLight(in AmbientLight _light, in vec3 _eyeToPoint, in vec3 _normal) {
    light_accumulator_ambient += _light.ambient;
}
