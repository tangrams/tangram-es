#header-only library

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO mapbox/geojson-cpp
    REF v0.3.2
    SHA512 4bda8d68f6425dd11b3ee4e2b39b55b9993abbab859519430046d0a131a1074de27f0449c5b852802b30214accebef4d8087c5e1f4a79ecc70871f05a3111016
    HEAD_REF master
    PATCHES 
        tangram.patch
)

# Copy header files
file(COPY ${SOURCE_PATH}/include/mapbox/ DESTINATION ${CURRENT_PACKAGES_DIR}/include/mapbox FILES_MATCHING PATTERN "*.hpp")

# Handle copyright
file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
