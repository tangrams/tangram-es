# set for test in other cmake files
set(PLATFORM_OSX ON)

check_unsupported_compiler_version()

# options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -stdlib=libc++ -std=c++1y")
set(CXX_FLAGS_DEBUG "-g -O0")


if (USE_EXTERNAL_LIBS)
    include(${EXTERNAL_LIBS_DIR}/core-dependencies.cmake)
    include(${EXTERNAL_LIBS_DIR}/glfw.cmake)
else()
    # build dependencies (yaml, benchmark, glfw)
    add_subdirectory(${PROJECT_SOURCE_DIR}/external)
endif()

# compile definitions (adds -DPLATFORM_OSX)
set(CORE_COMPILE_DEFS PLATFORM_OSX)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)

if(APPLICATION)
    set(EXECUTABLE_NAME "tangram")

    find_package(OpenGL REQUIRED)

    # add sources and include headers
    set(OSX_EXTENSIONS_FILES *.mm *.cpp)
    foreach(_ext ${OSX_EXTENSIONS_FILES})
        find_sources_and_include_directories(
            ${PROJECT_SOURCE_DIR}/platform/osx/src/*.h
            ${PROJECT_SOURCE_DIR}/platform/osx/src/${_ext})
    endforeach()

    add_bundle_resources(RESOURCES "${PROJECT_SOURCE_DIR}/core/resources" "Resources")
    add_bundle_resources(RESOURCES "${PROJECT_SOURCE_DIR}/scenes" "Resources")

    file(GLOB_RECURSE OSX_RESOURCES "${PROJECT_SOURCE_DIR}/platform/osx/resources/**")
    string(REGEX REPLACE "[.]DS_Store" "" OSX_RESOURCES "${OSX_RESOURCES}")

    include_directories(${PROJECT_SOURCE_DIR}/platform/common)

    add_executable(${EXECUTABLE_NAME} MACOSX_BUNDLE ${SOURCES} ${RESOURCES} ${OSX_RESOURCES})
    target_link_libraries(${EXECUTABLE_NAME} ${CORE_LIBRARY} glfw ${OPENGL_LIBRARIES})

    # add resource files and property list
    set_target_properties(${EXECUTABLE_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${PROJECT_SOURCE_DIR}/platform/osx/resources/tangram-Info.plist"
        RESOURCE "${OSX_RESOURCES}")

endif()
