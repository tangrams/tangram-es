#ifdef GL_ES
precision mediump float;
#endif

uniform float u_time;

varying vec4 v_pos;
varying vec4 v_color;
varying vec2 v_texcoord;

void main(void) {
	vec4 color = v_color;
	
	color.rg = v_texcoord.xy;

  	gl_FragColor = color;
}
