macro(vcpkg_setup)
    option(USE_VCPKG "enable vcpkg dependency build" ON)
    # needs before first project() call
    set(VCPKG_OVERLAY_PORTS
    "${CMAKE_CURRENT_LIST_DIR}/vcpkg-ports/mapbox-geojson-vt-cpp" 
    "${CMAKE_CURRENT_LIST_DIR}/vcpkg-ports/mapbox-geojson-cpp"
    "${CMAKE_CURRENT_LIST_DIR}/vcpkg-ports/mapbox-geometry"
    "${CMAKE_CURRENT_LIST_DIR}/vcpkg-ports/mapbox-variant"
    "${CMAKE_CURRENT_LIST_DIR}/vcpkg-ports/glm"
    )
    if(USE_VCPKG AND NOT DEFINED CMAKE_TOOLCHAIN_FILE)
        set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_LIST_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
    endif()
    if (VCPKG_TARGET_ANDROID)
    include("cmake/vcpkg_android.cmake")
    endif()
endmacro()
