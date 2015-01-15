#ifdef GL_ES
precision mediump flo

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

vec4 calculateLighting(in vec3 _eyeToPoint, in vec3 _normal) {
    vec3 eye = vec3(0.0, 0.0, 1.0);

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

    color.r = clamp(0.0,1.0,color.r);
    color.g = clamp(0.0,1.0,color.g);
    color.b = clamp(0.0,1.0,color.b);
    color.a = clamp(0.0,1.0,color.a);

    return color;
}

at;
#endif

uniform mat4 u_modelView;
uniform mat4 u_modelViewProj;
uniform float u_time;

#pragma tangram: defines

#ifndef TANGRAM_MATERIAL_DIFFUSE
#define TANGRAM_MATERIAL_DIFFUSE
#endif



#ifndef TANGRAM_VERTEX_LIGHTS
#define TANGRAM_VERTEX_LIGHTS
#endif

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

    color.r = clamp(0.0,1.0,color.r);
    color.g = clamp(0.0,1.0,color.g);
    color.b = clamp(0.0,1.0,color.b);
    color.a = clamp(0.0,1.0,color.a);
    
    return color;
#endif
}


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

    float nDotVP =  min( max(0.0, dot(_normal, normalize( vec3(_light.direction) ) ) ), 1.0);

    #ifdef TANGRAM_MATERIAL_DIFFUSE 
    g_light_accumulator_diffuse += _light.diffuse * nDotVP;
    #endif

    #ifdef TANGRAM_MATERIAL_SPECULAR
    float pf = 0.0;
    if (nDotVP != 0.0){
        vec3 halfVector = normalize( vec3(_light.direction) + _eye );
        float nDotHV = min( max(0.0, dot(_normal, halfVector) ), 1.0 );
        pf = pow(nDotHV, g_material.shininess);
    }
    g_light_accumulator_specular += _light.specular * pf;
    #endif
}


DirectionalLight g_dLight = DirectionalLight(vec4(0.00000000,0.00000000,0.00000000,0.00000000), vec4(1.00000000,1.00000000,1.00000000,1.00000000), vec4(0.00000000,0.00000000,0.00000000,0.00000000), vec3(-1.00000000,-1.00000000,1.00000000));


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
