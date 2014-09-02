
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

android/libs/armeabi/libtangram.so: android/jni/jniExports.cpp shared/tangram.cpp shared/tangram.h
	ndk-build -C android/jni

android/bin/TangramAndroid-Debug.apk: android/libs/armeabi/libtangram.so android/src/com/mapzen/tangram/*.java android/build.xml
	ant -f android/build.xml debug

android: android/bin/TangramAndroid-Debug.apk

ios:
	xcodebuild -workspace ios/TangramiOS.xcworkspace -scheme TangramiOS -destination 'platform=iOS Simulator,name=iPhone Retina (3.5-inch)'

osx/lib/libtangram.so: shared/tangram.cpp shared/tangram.h
	cc -o osx/obj/libtangram.so shared/tangram.cpp -DPLATFORM_OSX -lglfw3 -framework OpenGL -shared

osx/bin/TangramOSX: osx/lib/libtangram.so
	clang -o osx/bin/TangramOSX -Isrc/*.h osx/src/main.cpp osx/lib/libtangram.so -DPLATFORM_OSX -lglfw3 -framework Cocoa -framework IOKit -framework OpenGL -framework CoreVideo

osx: osx/bin/TangramOSX
