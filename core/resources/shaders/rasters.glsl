#ifdef TANGRAM_FRAGMENT_SHADER

uniform sampler2D u_rasters[TANGRAM_NUM_RASTER_SOURCES];
uniform vec2 u_raster_sizes[TANGRAM_NUM_RASTER_SOURCES];
uniform vec3 u_raster_offsets[TANGRAM_NUM_RASTER_SOURCES];

#define currentRasterUV(raster_index) \
    (v_modelpos_base_zoom.xy)

#define sampleRaster(raster_index) \
    (texture2D(u_rasters[raster_index], currentRasterUV(raster_index)))

#endif
