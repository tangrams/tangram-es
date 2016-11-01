tar -czvf packaging/deps.tar.gz \
	--anchored external/alfons --exclude external/alfons/fonts \
	--anchored external/geojson-vt-cpp \
	--anchored external/css-color-parser-cpp \
	--anchored external/duktape \
	--anchored external/yaml-cpp --exclude external/yaml-cpp/test \
	--anchored core/include/earcut.hpp --exclude core/include/earcut.hpp/glfw --exclude core/include/earcut.hpp/test \
	--anchored core/include/variant \
	--anchored core/include/isect2d \
	--anchored core/include/glm --exclude core/include/glm/doc --exclude core/include/glm/test
