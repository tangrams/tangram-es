tar -czvf packaging/deps.tar.gz \
	--anchored core/deps/alfons \
	--anchored core/deps/css-color-parser-cpp \
	--anchored core/deps/duktape \
	--anchored core/deps/earcut --exclude core/deps/earcut/glfw \
	--anchored core/deps/geojson-vt-cpp --exclude core/deps/geojson-vt-cpp/.mason \
	--anchored core/deps/isect2d \
	--anchored core/deps/SQLiteCpp --exclude core/deps/SQLiteCpp/googletest \
	--anchored core/deps/variant --exclude core/deps/variant/.mason \
	--anchored core/deps/yaml-cpp --exclude core/deps/yaml-cpp/test
