#ifdef GL_ES
precision mediump float;
#endif

uniform float u_time;

varying vec4 v_pos;
varying vec4 v_color;
varying vec3 v_eye;
varying vec3 v_normal;
varying vec2 v_texcoord;

void main(void) {
    
    /* 
     * Use texture coordinates to darken fragments closer to the center of the polyline,
     * creating an outline effect.
     */
    
    float radius = abs(v_texcoord.x - 0.5); // distance from center of line in texture coordinates
    
    float threshold = 0.35; // distance within which color will be darkened
    
    float darken = step(threshold, radius); // 0.0 if radius < threshold, else 1.0
    
    darken = clamp(darken, 0.5, 1.0); // reduce color values by 1/2 in darkened fragments
    
  	gl_FragColor = v_color * darken;
	gl_FragColor.a = 1.0;    
}
