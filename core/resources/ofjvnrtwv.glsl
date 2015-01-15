#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 u_modelView;
uniform mat4 u_modelViewProj;
uniform float u_time;

#pragma tangram: defines
#define TANGRAM_FRAGMENT_LIGHTS


#define TANGRAM_VERTEX_LIGHTS


#define TANGRAM_VERTEX_LIGHTS



#define TANGRAM_MATERIAL_DIFFUSE

#pragma tangram: material
// MATERIALS
//
struct Material {
    #ifdef TANGRAM_MATERIAL_EMISSION
    vec4 emission;
    #endif

    #ifdef TANGRAM_MATERIAL_AMBIENT
    vec4 ambient;
    #endif 

    #ifdef TANGRAM_MATERIAL_DIFFUSE
    vec4 diffuse;
    #endif

    #ifdef TANGRAM_MATERIAL_SPECULAR
    vec4 specular;
    float shininess;
    #endif
};

// Note: uniforms (u_[name]) and varyings (v_[name]) are 
//      copy to global instances ( g_[name] ) to allow 
//      modifications
//
uniform Material u_material;
Material g_material = u_material;

// GLOBAL LIGHTS ACCUMULATORS for each enable MATERIAL property
//
#ifdef TANGRAM_MATERIAL_AMBIENT
vec4 g_light_accumulator_ambient;
#endif
#ifdef TANGRAM_MATERIAL_DIFFUSE
vec4 g_light_accumulator_diffuse;
#endif
#ifdef TANGRAM_MATERIAL_SPECULAR
vec4 g_light_accumulator_specular;
#endif


#pragma tangram: _vertex_lighting
struct PointLight {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    vec4 position;

    #ifdef TANGRAM_POINTLIGHT_CONSTANT_ATTENUATION
    #define TANGRAM_POINTLIGHT_ATTENUATION
    float constantAttenuation;
    #endif

    #ifdef TANGRAM_POINTLIGHT_LINEAR_ATTENUATION
    #ifndef TANGRAM_POINTLIGHT_ATTENUATION
    #define TANGRAM_POINTLIGHT_ATTENUATION
    #endif
    #define TANGRAM_POINTLIGHT_DISTANCE
    float linearAttenuation;
    #endif


    #ifdef TANGRAM_POINTLIGHT_QUADRATIC_ATTENUATION
    #ifndef TANGRAM_POINTLIGHT_ATTENUATION
    #define TANGRAM_POINTLIGHT_ATTENUATION
    #endif
    #ifndef TANGRAM_POINTLIGHT_DISTANCE
    #define TANGRAM_POINTLIGHT_DISTANCE
    #endif
    float quadraticAttenuation;
    #endif
};

void calculateLight(in PointLight _light, in vec3 _eye, in vec3 _eyeToPoint, in vec3 _normal){

    // Compute vector from surface to light position
    vec3 VP = normalize(vec3(_light.position) - _eyeToPoint);

    #ifdef TANGRAM_POINTLIGHT_DISTANCE
    float dist = length(vec3(_light.position) - _eyeToPoint);
    #endif 

    // Normalize the vector from surface to light position
    float nDotVP = clamp(dot(VP, _normal), 0.0, 1.0);

    #ifdef TANGRAM_POINTLIGHT_ATTENUATION
    float atFactor = 0.0;

    #ifdef TANGRAM_POINTLIGHT_CONSTANT_ATTENUATION
    atFactor += _light.constantAttenuation;
    #endif

    #ifdef TANGRAM_POINTLIGHT_LINEAR_ATTENUATION
    atFactor += _light.linearAttenuation * dist;
    #endif

    #ifdef TANGRAM_POINTLIGHT_QUADRATIC_ATTENUATION
    atFactor += _light.quadraticAttenuation * dist * dist;
    #endif
    
    float attenuation = 1.0;
    if (atFactor != 0.0) {
        attenuation /= atFactor;
    }
    #endif

    #ifdef TANGRAM_MATERIAL_AMBIENT
    #ifdef TANGRAM_POINTLIGHT_ATTENUATION
    g_light_accumulator_ambient += _light.ambient * attenuation;
    #else
    g_light_accumulator_ambient += _light.ambient;
    #endif
    #endif
    
    #ifdef TANGRAM_MATERIAL_DIFFUSE 
    #ifdef TANGRAM_POINTLIGHT_ATTENUATION
    g_light_accumulator_diffuse += _light.diffuse * nDotVP * attenuation;
    #else
    g_light_accumulator_diffuse += _light.diffuse * nDotVP;
    #endif
    #endif

    #ifdef TANGRAM_MATERIAL_SPECULAR
    float pf = 0.0; // power factor for shinny speculars
    if (nDotVP > 0.0) {
        vec3 halfVector = normalize(VP + _eye); // Direction of maximum highlights
        float nDotHV = dot(_normal, halfVector);
        pf = pow(nDotHV, g_material.shininess);
    }

    #ifdef TANGRAM_POINTLIGHT_ATTENUATION
    g_light_accumulator_specular += _light.specular * pf * attenuation;
    #else
    g_light_accumulator_specular += _light.specular * pf;
    #endif

    #endif
}


uniform PointLight u_pLight;
PointLight g_pLight = u_pLight;

struct DirectionalLight {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;

    vec3 direction;
};

void calculateLight(in DirectionalLight _light, in vec3 _eye, in vec3 _eyeToPoint, in vec3 _normal){
    #ifdef TANGRAM_MATERIAL_AMBIENT
    g_light_accumulator_ambient += _light.ambient;
    #endif

    float nDotVP = dot(_normal, normalize(_light.direction));

    #ifdef TANGRAM_MATERIAL_DIFFUSE 
    g_light_accumulator_diffuse += _light.diffuse * nDotVP;
    #endif

    #ifdef TANGRAM_MATERIAL_SPECULAR
    float pf = 0.0;
    if (nDotVP != 0.0){
        vec3 halfVector = normalize(_light.direction + _eye );
        float nDotHV = dot(_normal, halfVector);
        pf = pow(nDotHV, g_material.shininess);
    }
    g_light_accumulator_specular += _light.specular * pf;
    #endif
}


DirectionalLight g_dLight = DirectionalLight(vec4(0.00000000,0.00000000,0.00000000,0.00000000), vec4(1.00000000,1.00000000,1.00000000,1.00000000), vec4(0.00000000,0.00000000,0.00000000,0.00000000), vec3(-1.00000000,-1.00000000,1.00000000));


// TODO: 
//      - BM - port this to STRINGIFY on C++

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

#ifdef TANGRAM_FRAGMENT_LIGHTS
void calculateLighting(in vec3 _eyeToPoint, in vec3 _normal ) {
#else
vec4 calculateLighting(in vec3 _eyeToPoint, in vec3 _normal ) {
#endif

    vec3 eye = vec3(0.0, 0.0, 1.0);

    #ifdef TANGRAM_MATERIAL_AMBIENT
    g_light_accumulator_ambient = vec4(0.0);
    #endif

    #ifdef TANGRAM_MATERIAL_DIFFUSE
    g_light_accumulator_diffuse = vec4(0.0);
    #endif

    #ifdef TANGRAM_MATERIAL_SPECULAR
    g_light_accumulator_specular = vec4(0.0);
    #endif

#pragma tangram: vertex_lights_to_compute
calculateLight(g_pLight, eye, _eyeToPoint, _normal);

calculateLight(g_dLight, eye, _eyeToPoint, _normal);


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
    
    color.r = clamp(color.r, 0.0, 1.0);
    color.g = clamp(color.g, 0.0, 1.0);
    color.b = clamp(color.b, 0.0, 1.0);
    color.a = clamp(color.a, 0.0, 1.0);

    return color;
#endif
}



attribute vec4 a_position;
attribute vec4 a_color;
attribute vec3 a_normal;
attribute vec2 a_texcoord;

varying vec4 v_color;
varying vec3 v_eyeToPoint;
varying vec3 v_normal;
varying vec2 v_texcoord;

void main() {

    v_normal = normalize(a_normal);
    v_texcoord = a_texcoord;

    v_eyeToPoint = vec3(u_modelView * a_position);

    v_color = a_color;

#ifdef TANGRAM_VERTEX_LIGHTS
    #ifdef TANGRAM_FRAGMENT_LIGHTS
    calculateLighting(v_eyeToPoint, v_normal);
    #else
    v_color *= calculateLighting(v_eyeToPoint, v_normal);
    #endif
#endif

    gl_Position = u_modelViewProj * a_position;

    
}