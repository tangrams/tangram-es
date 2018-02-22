#pragma tangram: extensions

#ifdef GL_ES
precision mediump float;
#define LOWP lowp
#else
#define LOWP
#endif

#pragma tangram: defines

uniform LOWP int u_sprite_mode;

#pragma tangram: uniforms

attribute vec2 a_uv;
attribute LOWP float a_alpha;
attribute LOWP vec4 a_color;
attribute vec4 a_position;
attribute vec4 a_outline_color;
attribute float a_aa_factor;

#ifdef TANGRAM_FEATURE_SELECTION
attribute vec4 a_selection_color;
varying vec4 v_selection_color;
#endif

varying vec4 v_color;
varying vec2 v_texcoords;
varying float v_aa_factor;
varying vec2 v_edge;
varying vec4 v_outline_color;
varying float v_alpha;

#pragma tangram: global

void main() {

    v_alpha = a_alpha;
    v_color = a_color;

#ifdef TANGRAM_FEATURE_SELECTION
    v_selection_color = a_selection_color;
    // Skip non-selectable meshes
    if (v_selection_color == vec4(0.0)) {
        gl_Position = vec4(0.0);
        return;
    }
#endif

    if (u_sprite_mode == 0) {
        v_texcoords = sign(a_uv);
        v_edge = abs(a_uv);
    } else {
        v_texcoords = a_uv;
    }
    v_outline_color = a_outline_color;
    v_aa_factor = a_aa_factor;

    gl_Position = a_position;
}
