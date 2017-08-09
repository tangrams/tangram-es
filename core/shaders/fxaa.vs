#ifdef GL_ES
precision mediump float;
#endif

uniform mat4 u_proj;

//texcoords computed in vertex step
//to avoid dependent texture reads
varying vec2 v_rgbNW;
varying vec2 v_rgbNE;
varying vec2 v_rgbSW;
varying vec2 v_rgbSE;
varying vec2 v_rgbM;

//a resolution for our optimized shader
uniform vec2 u_resolution;
attribute vec2 a_position;
attribute vec2 a_uv;
varying vec2 vUv;

void texcoords(vec2 fragCoord, vec2 resolution,
                        out vec2 v_rgbNW, out vec2 v_rgbNE,
                        out vec2 v_rgbSW, out vec2 v_rgbSE,
                        out vec2 v_rgbM) {
        vec2 inverseVP = 1.0 / resolution.xy;
        v_rgbNW = (fragCoord + vec2(-1.0, -1.0)) * inverseVP;
        v_rgbNE = (fragCoord + vec2(1.0, -1.0)) * inverseVP;
        v_rgbSW = (fragCoord + vec2(-1.0, 1.0)) * inverseVP;
        v_rgbSE = (fragCoord + vec2(1.0, 1.0)) * inverseVP;
        v_rgbM = vec2(fragCoord * inverseVP);
}

void main(void) {
  //gl_Position = vec4(a_uv, 1.0, 1.0);
  //gl_Position = vec4(a_position, 0.0, 1.0);
  gl_Position = u_proj * vec4(a_position, 1.0, 1.0);

  //compute the texture coords and send them to varyings
  vec2 pos = a_position / u_resolution;
  // vUv = (pos - 0.5) * 2.0;
  // vUv.y = 1.0 - vUv.y;
  vUv = a_uv;
  //vUv = pos;
  vec2 fragCoord = vUv * u_resolution;
  texcoords(fragCoord, u_resolution, v_rgbNW, v_rgbNE, v_rgbSW, v_rgbSE, v_rgbM);
}
