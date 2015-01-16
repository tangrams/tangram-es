all: android osx osx-xcode ios

.PHONY: clean
.PHONY: clean-android
.PHONY: clean-osx
.PHONY: clean-osx-xcode
.PHONY: clean-ios
.PHONY: clean-rpi
.PHONY: android
.PHONY: osx
.PHONY: osx-xcode
.PHONY: ios
.PHONY: rpi
.PHONY: check-ndk
.PHONY: cmake-osx
.PHONY: cmake-osx-xcode
.PHONY: cmake-android
.PHONY: cmake-ios
.PHONY: cmake-rpi
.PHONY: install-android

ANDROID_BUILD_DIR = build/android
OSX_BUILD_DIR = build/osx
OSX_XCODE_BUILD_DIR = build/osx-xcode
IOS_BUILD_DIR = build/ios
RPI_BUILD_DIR = build/rpi
TESTS_BUILD_DIR = build/tests
UNIT_TESTS_BUILD_DIR = ${TESTS_BUILD_DIR}/unit

TOOLCHAIN_DIR = toolchains
OSX_TARGET = tangram
IOS_TARGET = tangram
OSX_XCODE_PROJ = tangram.xcodeproj
IOS_XCODE_PROJ = tangram.xcodeproj

ifndef ANDROID_ARCH
	ANDROID_ARCH = armeabi-v7a
endif

ifndef ANDROID_API_LEVEL
	ANDROID_API_LEVEL = android-19
endif

ifndef IOS_PLATFORM
	IOS_PLATFORM = SIMULATOR
endif

UNIT_TESTS_CMAKE_PARAMS = \
	-DUNIT_TESTS=1

ANDROID_CMAKE_PARAMS = \
	-DPLATFORM_TARGET=android \
	-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_DIR}/android.toolchain.cmake \
	-DMAKE_BUILD_TOOL=$$ANDROID_NDK/prebuilt/darwin-x86_64/bin/make \
	-DANDROID_ABI=${ANDROID_ARCH} \
	-DANDROID_STL=c++_shared \
	-DANDROID_NATIVE_API_LEVEL=${ANDROID_API_LEVEL}

IOS_CMAKE_PARAMS = \
	-DPLATFORM_TARGET=ios \
	-DIOS_PLATFORM=${IOS_PLATFORM} \
	-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_DIR}/iOS.toolchain.cmake \
	-G Xcode

DARWIN_XCODE_CMAKE_PARAMS = \
	-DPLATFORM_TARGET=darwin \
	-G Xcode

DARWIN_CMAKE_PARAMS = \
	-DPLATFORM_TARGET=darwin

RPI_CMAKE_PARAMS = \
	-DPLATFORM_TARGET=raspberrypi \
	-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_DIR}/raspberrypi.toolchain.cmake

clean: clean-android clean-osx clean-ios clean-rpi clean-tests clean-osx-xcode

clean-android:
	ant -f android/build.xml clean
	rm -rf ${ANDROID_BUILD_DIR}
	rm -rf android/libs/${ANDROID_ARCH} android/obj

clean-osx:
	rm -rf ${OSX_BUILD_DIR}
	
clean-ios:
	rm -rf ${IOS_BUILD_DIR}

clean-rpi:
	rm -rf ${RPI_BUILD_DIR}

clean-osx-xcode:
	rm -rf ${OSX_XCODE_BUILD_DIR}

clean-tests:
	rm -rf ${TESTS_BUILD_DIR}

android: install-android android/libs/${ANDROID_ARCH}/libtangram.so android/build.xml
	ant -q -f android/build.xml debug

install-android: check-ndk cmake-android ${ANDROID_BUILD_DIR}/Makefile
	cd ${ANDROID_BUILD_DIR} && \
	${MAKE} && \
	${MAKE} install

cmake-android:
	mkdir -p ${ANDROID_BUILD_DIR} 
	cd ${ANDROID_BUILD_DIR} && \
	cmake ../.. ${ANDROID_CMAKE_PARAMS}

osx: cmake-osx ${OSX_BUILD_DIR}/Makefile
	cd ${OSX_BUILD_DIR} && \
	${MAKE}

osx-xcode: cmake-osx-xcode ${OSX_XCODE_BUILD_DIR}
	xcodebuild -target ${OSX_TARGET} -project ${OSX_XCODE_BUILD_DIR}/${OSX_XCODE_PROJ}

cmake-osx-xcode:
ifeq ($(wildcard ${OSX_XCODE_BUILD_DIR}/${OSX_XCODE_PROJ}/.*),)
	mkdir -p ${OSX_XCODE_BUILD_DIR} 
	cd ${OSX_XCODE_BUILD_DIR} && \
	cmake ../.. ${DARWIN_XCODE_CMAKE_PARAMS}
endif

cmake-osx: 
	mkdir -p ${OSX_BUILD_DIR} 
	cd ${OSX_BUILD_DIR} && \
	cmake ../.. ${DARWIN_CMAKE_PARAMS}

ios: cmake-ios ${IOS_BUILD_DIR}/${IOS_XCODE_PROJ}
	xcodebuild -target ${IOS_TARGET} -project ${IOS_BUILD_DIR}/${IOS_XCODE_PROJ}

cmake-ios:
ifeq ($(wildcard ${IOS_BUILD_DIR}/${IOS_XCODE_PROJ}/.*),)
	mkdir -p ${IOS_BUILD_DIR}
	cd ${IOS_BUILD_DIR} && \
	cmake ../.. ${IOS_CMAKE_PARAMS}
endif

rpi: cmake-rpi ${RPI_BUILD_DIR}
	cd ${RPI_BUILD_DIR} && \
	${MAKE}
	
cmake-rpi:
	mkdir -p ${RPI_BUILD_DIR}
	cd ${RPI_BUILD_DIR} && \
	cmake ../.. ${RPI_CMAKE_PARAMS}
	
tests: unit-tests

unit-tests:
	mkdir -p ${UNIT_TESTS_BUILD_DIR} 
	cd ${UNIT_TESTS_BUILD_DIR} && \
	cmake ../../.. ${UNIT_TESTS_CMAKE_PARAMS} && \
	${MAKE}

check-ndk:
ifndef ANDROID_NDK
	$(error ANDROID_NDK is undefined)
endif

