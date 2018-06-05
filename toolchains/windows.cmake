# global compile options
if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xclang -std=gnu++1y")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DRAPIDJSON_HAS_CXX11_RVALUE_REFS=1")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc") # Exceptions handler - required for yaml-cpp
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /D_USE_MATH_DEFINES") # required for geojson-vt-cpp
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++1y")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wignored-qualifiers -Wtype-limits -Wmissing-field-initializers")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fstack-protector")
	# Static build
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static -DCURL_STATICLIB")
	
	if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		# Clang may be also used in MSVC-Clang toolchain and that will pass match above,
		# But then clang works in clang-cl mode which emulates cl.exe and accepts MSVC args, not GNU-like
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-gnu-zero-variadic-macro-arguments")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
		set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}  -lc++ -lc++abi")
	endif()
endif()

if (CMAKE_COMPILER_IS_GNUCC)
  execute_process(COMMAND ${CMAKE_CXX_COMPILER} -dumpversion
    OUTPUT_VARIABLE GCC_VERSION)
  string(REGEX MATCHALL "[0-9]+" GCC_VERSION_COMPONENTS ${GCC_VERSION})
  list(GET GCC_VERSION_COMPONENTS 0 GCC_MAJOR)
  list(GET GCC_VERSION_COMPONENTS 1 GCC_MINOR)

  message(STATUS "Using gcc ${GCC_VERSION}")
  if (GCC_VERSION VERSION_GREATER 5.1)
    message(STATUS "USE CXX11_ABI")
    add_definitions("-D_GLIBCXX_USE_CXX11_ABI=1")
  endif()
endif()

check_unsupported_compiler_version()

add_definitions(-DTANGRAM_WINDOWS)

# load core library
add_subdirectory(${PROJECT_SOURCE_DIR}/core)

if(TANGRAM_APPLICATION)

  set(EXECUTABLE_NAME "tangram")

  get_nextzen_api_key(NEXTZEN_API_KEY)
  add_definitions(-DNEXTZEN_API_KEY="${NEXTZEN_API_KEY}")

  if($ENV{CIRCLE_BUILD_NUM})
    add_definitions(-DBUILD_NUM_STRING="\($ENV{CIRCLE_BUILD_NUM}\)")
  endif()
	
  find_package(OpenGL REQUIRED)
	find_package(CURL REQUIRED)

  # Build GLFW.
  if (TANGRAM_USE_SYSTEM_GLFW_LIBS)
    include(FindPkgConfig)
    pkg_check_modules(GLFW REQUIRED glfw3)
  else()
    # configure GLFW to build only the library
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
    set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")
    add_subdirectory(${PROJECT_SOURCE_DIR}/platforms/common/glfw)
  endif()
	
  add_executable(${EXECUTABLE_NAME}
    ${PROJECT_SOURCE_DIR}/platforms/windows/src/windowsPlatform.cpp
    ${PROJECT_SOURCE_DIR}/platforms/windows/src/main.cpp
    ${PROJECT_SOURCE_DIR}/platforms/common/platform_gl.cpp
    ${PROJECT_SOURCE_DIR}/platforms/common/urlClient.cpp
    ${PROJECT_SOURCE_DIR}/platforms/common/glfwApp.cpp
    ${PROJECT_SOURCE_DIR}/platforms/common/glad.c
	)
		
  target_include_directories(${EXECUTABLE_NAME}
    PUBLIC
    ${GLFW_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/platforms/common
		${CURL_INCLUDE_DIRS}
	)

  target_link_libraries(${EXECUTABLE_NAME}
    ${CORE_LIBRARY}
		glfw
    # only used when not using external lib
    -pthread
    ${GLFW_LIBRARIES}
    ${OPENGL_LIBRARIES}
		${CURL_LIBRARIES}
	)
	# For CURL static
	target_link_libraries(${EXECUTABLE_NAME}
		wsock32 ws2_32 crypt32 wldap32
	)

  add_resources(${EXECUTABLE_NAME} "${PROJECT_SOURCE_DIR}/scenes")

endif()
