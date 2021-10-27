vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO hjanetzek/alfons
    REF 26a4dc693ad35609d4909141f763f1bad32e681f
    SHA512 afba6b007ea7814d660577ace320b501ef3ae3b5b19a4614ec3564a3368b888aaa8340087998d691976fa24eaddbe58f14ad1b114b29e784392d3a3e79c63b22
    HEAD_REF master
    PATCHES 
        cmake.patch
)

file(COPY ${CMAKE_CURRENT_LIST_DIR}/Config.cmake.in DESTINATION ${SOURCE_PATH}/cmake)

vcpkg_configure_cmake(
    SOURCE_PATH ${SOURCE_PATH}
    PREFER_NINJA
)

vcpkg_install_cmake()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/alfons)
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

# Put the license file where vcpkg expects it
file(INSTALL ${SOURCE_PATH}/LICENSE DESTINATION ${CURRENT_PACKAGES_DIR}/share/${PORT} RENAME copyright)
