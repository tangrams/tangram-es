#ifdef GL_ES
precision highp float;
#endif

varying vec4 v_world_position;

void main(void) {
    //if (v_world_position.z < 0.0) {
        gl_FragColor = vec4(1.0);
    //} else {
    //    discard;
    //}
}
