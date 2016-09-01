
#ifdef GL_ES
precision highp float;
#endif

varying vec4 v_selection_color;

void main(void) {
    gl_FragColor = v_selection_color;
}
