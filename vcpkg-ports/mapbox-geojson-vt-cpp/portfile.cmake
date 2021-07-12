#header-only library

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO sopyer/geojson-vt-cpp
    REF 060960748e448c1ddb507b2cce099ab5cc330ba2
    SHA512 cf970de3c81169f4b9458cc772399e8c68c3433d3596d35a67de7ae64daef6c52ed1ba6da51d4e5ef4b5fc8603ca60f976112595a2da1c5a2f022adabba6b204
    HEAD_REF master
)

# Copy header files
file(COPY ${SOURCE_PATH}/include/mapbox/ DESTINATION ${CURRENT_PACKAGES_DIR}/include/mapbox FILES_MATCHING PATTERN "*.hpp")

# Handle copyright
file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
