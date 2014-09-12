
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

android/libs/armeabi/libtangram.so: android/jni/jniExports.cpp android/jni/platform_android.cpp core/tangram.cpp core/tangram.h android/jni/Android.mk android/jni/Application.mk
	ndk-build -C android/jni

android/bin/TangramAndroid-Debug.apk: android/libs/armeabi/libtangram.so android/src/com/mapzen/tangram/*.java android/build.xml
	ant -f android/build.xml debug

android: android/bin/TangramAndroid-Debug.apk

ios:
	xcodebuild -workspace ios/TangramiOS.xcworkspace -scheme TangramiOS -destination 'platform=iOS Simulator,name=iPhone Retina (3.5-inch)'

osx/lib/libtangram.so: core/tangram.cpp core/tangram.h
	clang++ -o osx/lib/libtangram.so core/tangram.cpp core/viewModule/viewModule.cpp osx/src/platform_osx.cpp -Icore -Icore/include -DPLATFORM_OSX -lglfw3 -framework OpenGL -shared -std=c++11

osx/bin/TangramOSX: osx/lib/libtangram.so
	clang++ -o osx/bin/TangramOSX osx/src/main.cpp osx/lib/libtangram.so -Icore/include -DPLATFORM_OSX -lglfw3 -framework Cocoa -framework IOKit -framework OpenGL -framework CoreVideo

osx: osx/bin/TangramOSX
