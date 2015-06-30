#ifdef GL_ES
precision highp float;
#endif

uniform mat4 u_modelView;
uniform mat4 u_modelViewProj;
uniform mat3 u_normalMatrix;
uniform float u_time;

varying vec4 v_color;
varying vec3 v_eyeToPoint;
varying vec3 v_normal;
varying vec2 v_texcoord;

#ifdef TANGRAM_LIGHTING_VERTEX
    varying vec4 v_lighting;
#endif

#pragma tangram: material
#pragma tangram: lighting
#pragma tangram: globals

void main(void) {
    vec4 color = v_color;
    vec3 normal = v_normal;

    #ifdef TANGRAM_MATERIAL_NORMAL_TEXTURE
        calculateNormal(normal);
    #endif

    // Modify normal before lighting
    #pragma tangram: normal

    // Modify color and material properties before lighting
    #ifndef TANGRAM_LIGHTING_VERTEX
        #pragma tangram: color
    #endif

    #ifdef TANGRAM_LIGHTING_FRAGMENT
        color = calculateLighting(v_eyeToPoint.xyz, normal, color);
    #else
        #ifdef TANGRAM_LIGHTING_VERTEX
            color = v_lighting;
        #endif
    #endif

    // Modify color after lighting (filter-like effects that don't require a additional render passes)
    #pragma tangram: filter

    //color.rgb = pow(color.rgb, vec3(1.0/2.2)); // gamma correction
    gl_FragColor = color;
}
