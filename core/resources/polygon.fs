#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 u_modelView;
uniform mat4 u_modelViewProj;
uniform float u_time;

#pragma tangram: material

// Material g_material;

#pragma tangram: lighting

varying vec4 v_color;
varying vec3 v_ecPosition;
varying vec3 v_normal;
varying vec2 v_texcoord;

void main(void) {
	vec4 color = v_color;

	// if(isWindow){
	// 	g_material.emission = vec4(1.0,1.0,0.0,1.0);
	// 	g_material.diffuse = vec4(0.1,0.1,0.1,0.1);
	// 	g_material.specular = vec4(vec3(0.9),1.0);
	// } else {
	// 	g_material.emission = vec4(0.0);
	// 	g_material.diffuse = vec4(0.1,0.1,0.1,1.0);
	// 	g_material.specular = vec4(vec3(0.1),1.0);
	// }

	color *= calculateLighting(v_ecPosition,v_normal);

  	gl_FragColor = color;
}
