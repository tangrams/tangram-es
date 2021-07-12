# header-only
vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO mapbox/variant
    REF v1.1.5
    SHA512 ba20bbe878f9bbe7d87292aaf7e51b98b24f77063f4cfd2e17d6841b7af52eb6c45228fb74a0b83824f4dfd3302f69b0ec711f483671ee57c903f9a666e3ce67
    HEAD_REF master
)

# Copy header files
file(COPY ${SOURCE_PATH}/include/mapbox/ DESTINATION ${CURRENT_PACKAGES_DIR}/include/mapbox FILES_MATCHING PATTERN "*.hpp")

# Handle copyright
file(COPY ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/mapbox-variant)
file(RENAME ${CURRENT_PACKAGES_DIR}/share/mapbox-variant/LICENSE ${CURRENT_PACKAGES_DIR}/share/mapbox-variant/copyright)
