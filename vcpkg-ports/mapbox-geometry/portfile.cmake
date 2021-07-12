#header-only library

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO mapbox/geometry.hpp
    REF v0.9.2
    SHA512 20144fe27c8de85603198fd5fa6dc877ed9ac3b2c735cc3d60b8cd34187c792b78010f78a6ed3a99224b5c00f95c169cd7ce2e6d063440341a192d9389dd9b42
    HEAD_REF master
    PATCHES 
        tangram.patch
)

# Copy header files
file(COPY ${SOURCE_PATH}/include/mapbox/ DESTINATION ${CURRENT_PACKAGES_DIR}/include/mapbox FILES_MATCHING PATTERN "*.hpp")

# Handle copyright
file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
