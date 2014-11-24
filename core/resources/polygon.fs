#pragma tangram: lighting

uniform float u_time;

varying vec4 v_color;
varying vec3 v_eye;

varying vec3 v_normal;
varying vec2 v_texcoord;

void main(void) {
	vec4 color = v_color;

	color *= calculateLighting(v_eye,v_normal);
	// color = color * u_directionalLights[0].diffuse * max(0.0, dot(v_normal, normalize(vec3(u_directionalLights[0].direction))));

  	gl_FragColor = color;
}
// 