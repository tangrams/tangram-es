
all: android osx

.PHONY: clean
.PHONY: clean-android
.PHONY: clean-ios
.PHONY: clean-osx
.PHONY: clean-osx-test
.PHONY: clean-osx-debug
.PHONY: ios
.PHONY: android
.PHONY: osx
.PHONY: osx-test
.PHONY: osx-debug

clean-android:
	ndk-build -C android/jni clean
	ant -f android/build.xml clean

clean-ios:
	xcodebuild -workspace ios/TangramiOS.xcworkspace -scheme TangramiOS clean

clean-osx:
	rm -rf osx/obj/*
	rm -rf osx/bin/*

clean-osx-test:
	rm -rf osx/obj/*
	rm -rf osx/tests/*

clean-osx-debug:
	rm -rf osx/obj/*
	rm -rf osx/debug/*

clean: clean-android clean-ios clean-osx clean-osx-test clean-osx-debug

CORE_SRC_FILES= \
	core/tangram.cpp \
	core/util/*.cpp \
	core/viewModule/*.cpp \
	core/dataSource/*.cpp \
	core/tileManager/*.cpp \
	core/mapTile/*.cpp

LIB_DEPENDENCY = \
	core/include/jsoncpp.cpp

CORE_TEST_FILES = \
	unitTests/tests/*.cpp

OSX_FRAMEWORKS= \
	-framework Cocoa \
	-framework OpenGL \
	-framework IOKit \
	-framework CoreVideo
OSX_INCLUDES= \
	-Icore \
	-Icore/include 
OSX_EXTERNAL_LIB= \
	-lcurl
OSX_SRC_FILES= \
	osx/src/main.cpp \
	osx/src/platform_osx.cpp
OSX_TEST_FILES= \
	osx/src/platform_osx.cpp

android/libs/armeabi/libtangram.so: android/jni/jniExports.cpp android/jni/platform_android.cpp core/tangram.cpp core/tangram.h android/jni/Android.mk android/jni/Application.mk
	ndk-build -C android/jni

android/bin/TangramAndroid-Debug.apk: android/libs/armeabi/libtangram.so android/src/com/mapzen/tangram/*.java android/build.xml
	mkdir -p android/bin
	ant -f android/build.xml debug

android: android/bin/TangramAndroid-Debug.apk

ios:
	xcodebuild -workspace ios/TangramiOS.xcworkspace -scheme TangramiOS -destination 'platform=iOS Simulator,name=iPhone Retina (3.5-inch)'

osx/bin/TangramOSX: $(OSX_SRC_FILES)
	mkdir -p osx/bin
	clang++ -o osx/bin/TangramOSX $(CORE_SRC_FILES) $(OSX_SRC_FILES) $(LIB_DEPENDENCY) $(OSX_INCLUDES) $(OSX_EXTERNAL_LIB) $(OSX_FRAMEWORKS) -DPLATFORM_OSX -lglfw3 -std=c++11

osx: osx/bin/TangramOSX

osx/debug/TangramOSX: $(OSX_SRC_FILES)
	mkdir -p osx/debug
	clang++ -o osx/debug/TangramOSX $(CORE_SRC_FILES) $(OSX_SRC_FILES) $(LIB_DEPENDENCY) $(OSX_INCLUDES) $(OSX_EXTERNAL_LIB) $(OSX_FRAMEWORKS) -DPLATFORM_OSX -lglfw3 -std=c++11 -g -DDEBUG

osx-debug: osx/debug/TangramOSX

osx/tests/TangramOSX: $(OSX_SRC_FILES) $(CORE_TEST_FILES) $(OSX_TEST_FILES)
	mkdir -p osx/tests
	clang++ -o osx/tests/TangramOSX $(CORE_SRC_FILES) $(CORE_TEST_FILES) $(OSX_TEST_FILES) $(LIB_DEPENDENCY) $(OSX_INCLUDES) $(OSX_EXTERNAL_LIB) $(OSX_FRAMEWORKS) -DPLATFORM_OSX -lglfw3 -std=c++11 -g -DDEBUG

osx-test: osx/tests/TangramOSX

