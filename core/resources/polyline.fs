#pragma tangram: lighting

uniform float u_time;

varying vec4 v_color;

varying vec3 v_eye;
varying vec3 v_normal;
varying vec2 v_texcoord;

void main(void) {
	vec4 color = v_color;
	
	// color *= calculateLighting(v_eye,v_normal);

  	gl_FragColor = color;
}
