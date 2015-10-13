#ifdef GL_ES
precision highp float;
#endif

uniform samplerCube u_tex;

varying vec3 v_uv;

void main() {

	gl_FragColor = textureCube(u_tex, v_uv);

}
