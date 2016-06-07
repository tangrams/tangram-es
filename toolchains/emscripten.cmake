# options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -std=c++1y -s USE_GLFW=3 -s FULL_ES2=1 -s EMULATE_FUNCTION_POINTER_CASTS=1 -s DISABLE_EXCEPTION_CATCHING=0")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --preload-file ${PROJECT_SOURCE_DIR}/scenes@/")
set(EXECUTABLE_NAME "tangram")

add_definitions(-DPLATFORM_JS)

if (USE_EXTERNAL_LIBS)
include(${EXTERNAL_LIBS_DIR}/core-dependencies.cmake)
include(${EXTERNAL_LIBS_DIR}/glfw.cmake)
else()
# build dependencies (yaml, benchmark, glfw)
add_subdirectory(${PROJECT_SOURCE_DIR}/external)
endif()

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)
include_directories(${CORE_INCLUDE_DIRS})

# add sources and include headers
set(JS_EXTENSIONS_FILES *.cpp)
foreach(_ext ${JS_EXTENSIONS_FILES})
    find_sources_and_include_directories(
        ${PROJECT_SOURCE_DIR}/emscripten/src/*.h
        ${PROJECT_SOURCE_DIR}/emscripten/src/${_ext})
endforeach()

if(APPLICATION)
    find_package(OpenGL REQUIRED)

    add_executable(${EXECUTABLE_NAME} ${SOURCES})
    target_link_libraries(${EXECUTABLE_NAME}
        ${CORE_LIBRARY}
        glfw
        ${OPENGL_LIBRARIES})
endif()
