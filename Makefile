
all: android osx

.PHONY: clean
.PHONY: clean-android
.PHONY: clean-ios
.PHONY: clean-osx
.PHONY: ios
.PHONY: android
.PHONY: osx

clean-android:
	ndk-build -C android/jni clean
	ant -f android/build.xml clean

clean-ios:
	xcodebuild -workspace ios/TangramiOS.xcworkspace -scheme TangramiOS clean

clean-osx:
	rm -rf osx/obj/*
	rm -rf osx/bin/*

clean: clean-android clean-ios clean-osx

CORE_SRC_FILES= \
	core/tangram.cpp \
	core/util/*.cpp \
	core/viewModule/*.cpp

OSX_FRAMEWORKS= \
	-framework Cocoa \
	-framework OpenGL \
	-framework IOKit \
	-framework CoreVideo
OSX_INCLUDES= \
	-Icore \
	-Icore/include
OSX_SRC_FILES= \
	osx/src/main.cpp \
	osx/src/platform_osx.cpp

android/libs/armeabi/libtangram.so: android/jni/jniExports.cpp android/jni/platform_android.cpp core/tangram.cpp core/tangram.h android/jni/Android.mk android/jni/Application.mk
	ndk-build -C android/jni

android/bin/TangramAndroid-Debug.apk: android/libs/armeabi/libtangram.so android/src/com/mapzen/tangram/*.java android/build.xml
	ant -f android/build.xml debug

android: android/bin/TangramAndroid-Debug.apk

ios:
	xcodebuild -workspace ios/TangramiOS.xcworkspace -scheme TangramiOS -destination 'platform=iOS Simulator,name=iPhone Retina (3.5-inch)'

osx/bin/TangramOSX: $(OSX_SRC_FILES)
	clang++ -o osx/bin/TangramOSX $(CORE_SRC_FILES) $(OSX_SRC_FILES) $(OSX_INCLUDES) $(OSX_FRAMEWORKS) -DPLATFORM_OSX -lglfw3 -std=c++11 -g

osx: osx/bin/TangramOSX
