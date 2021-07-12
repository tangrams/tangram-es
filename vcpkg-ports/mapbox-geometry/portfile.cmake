#header-only library

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO mapbox/geometry.hpp
    REF v0.9.3
    SHA512 01a045f93f5fdf6c477ce96374ed09471c16a61c4365d8a33765060a5c9ce619e8ce0196593cc9e699cd078daf7ebf7ca01423d26cd493a3455b07e8f6759302
    HEAD_REF master
    PATCHES 
        tangram.patch
)

# Copy header files
file(COPY ${SOURCE_PATH}/include/mapbox/ DESTINATION ${CURRENT_PACKAGES_DIR}/include/mapbox FILES_MATCHING PATTERN "*.hpp")

# Handle copyright
file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
