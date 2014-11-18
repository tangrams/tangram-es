#ifdef GL_ES
precision mediump float;
#endif

uniform float u_time;

varying vec4 v_pos;
varying vec4 v_color;

void main(void) {
	vec4 color = v_color;
	
	color.rgb = vec3(abs(cos(u_time+v_pos.x)),
					 abs(sin(u_time+v_pos.y)), 
					 0.5 );

  	gl_FragColor = color;
}
