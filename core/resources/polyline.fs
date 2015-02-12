#ifdef GL_ES
precision highp float;
#endif

uniform mat4 u_modelView;
uniform mat4 u_modelViewProj;
uniform float u_time;

varying vec4 v_pos;
varying vec4 v_color;
varying vec3 v_eyeToPoint;
varying vec3 v_normal;
varying vec2 v_texcoord;

void main(void) {
    
    vec4 color = v_color;

    /* 
     * Use texture coordinates to darken fragments closer to the center of the polyline,
     * creating an outline effect.
     */
    
    float radius = abs(v_texcoord.x - 0.5); // distance from center of line in texture coordinates
    
    float threshold = 0.35; // distance within which color will be darkened
    
    float darken = step(threshold, radius); // 0.0 if radius < threshold, else 1.0
    
    darken = clamp(darken, 0.5, 1.0); // reduce color values by 1/2 in darkened fragments

  	color.rgb = color.rgb * darken;
    //color.rgb = pow(color.rgb, vec3(1.0/2.2)); // gamma correction
	gl_FragColor = color;
}
