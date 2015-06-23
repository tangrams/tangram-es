all: android osx ios

.PHONY: clean
.PHONY: clean-android
.PHONY: clean-osx
.PHONY: clean-xcode
.PHONY: clean-ios
.PHONY: clean-rpi
.PHONY: clean-linux
.PHONY: android
.PHONY: osx
.PHONY: xcode
.PHONY: ios
.PHONY: ios-sim
.PHONY: rpi
.PHONY: linux
.PHONY: check-ndk
.PHONY: cmake-osx
.PHONY: cmake-xcode
.PHONY: cmake-android
.PHONY: cmake-ios
.PHONY: cmake-ios-sim
.PHONY: cmake-rpi
.PHONY: cmake-linux
.PHONY: install-android

ANDROID_BUILD_DIR = build/android
OSX_BUILD_DIR = build/osx
OSX_XCODE_BUILD_DIR = build/xcode
IOS_BUILD_DIR = build/ios
IOS_SIM_BUILD_DIR = build/ios-sim
RPI_BUILD_DIR = build/rpi
LINUX_BUILD_DIR = build/linux
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
	ANDROID_API_LEVEL = android-15
endif

UNIT_TESTS_CMAKE_PARAMS = \
	-DUNIT_TESTS=1

ANDROID_CMAKE_PARAMS = \
	-DPLATFORM_TARGET=android \
	-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_DIR}/android.toolchain.cmake \
	-DMAKE_BUILD_TOOL=$$ANDROID_NDK/prebuilt/linux-x86_64/bin/make \
	-DANDROID_TOOLCHAIN_NAME=arm-linux-androideabi-4.9 \
	-DANDROID_ABI=${ANDROID_ARCH} \
	-DANDROID_STL=gnustl_static \
	-DANDROID_NATIVE_API_LEVEL=${ANDROID_API_LEVEL}

IOS_CMAKE_PARAMS = \
	-DPLATFORM_TARGET=ios \
	-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_DIR}/iOS.toolchain.cmake \
	-G Xcode

DARWIN_XCODE_CMAKE_PARAMS = \
	-DPLATFORM_TARGET=darwin \
	-G Xcode

DARWIN_CMAKE_PARAMS = \
	-DPLATFORM_TARGET=darwin

RPI_CMAKE_PARAMS = \
	-DPLATFORM_TARGET=raspberrypi

LINUX_CMAKE_PARAMS = \
	-DPLATFORM_TARGET=linux

clean: clean-android clean-osx clean-ios clean-rpi clean-tests clean-xcode clean-linux

clean-android:
	@cd android/ && \
	./gradlew clean
	rm -rf ${ANDROID_BUILD_DIR}
	rm -rf android/libs android/obj

clean-osx:
	rm -rf ${OSX_BUILD_DIR}

clean-ios:
	rm -rf ${IOS_BUILD_DIR}

clean-rpi:
	rm -rf ${RPI_BUILD_DIR}

clean-linux:
	rm -rf ${LINUX_BUILD_DIR}

clean-xcode:
	rm -rf ${OSX_XCODE_BUILD_DIR}

clean-tests:
	rm -rf ${TESTS_BUILD_DIR}

android: android/libs/${ANDROID_ARCH}/libtangram.so android/build.gradle
	@cd android/ && \
	./gradlew assembleDebug

android/libs/${ANDROID_ARCH}/libtangram.so: install-android

install-android: ${ANDROID_BUILD_DIR}/Makefile
	@cd ${ANDROID_BUILD_DIR} && \
	${MAKE} && \
	${MAKE} install

${ANDROID_BUILD_DIR}/Makefile: check-ndk cmake-android

cmake-android:
	@mkdir -p ${ANDROID_BUILD_DIR}
	@cd ${ANDROID_BUILD_DIR} && \
	cmake ../.. ${ANDROID_CMAKE_PARAMS}

osx: ${OSX_BUILD_DIR}/Makefile
	@cd ${OSX_BUILD_DIR} && \
	${MAKE}

${OSX_BUILD_DIR}/Makefile: cmake-osx

xcode: ${OSX_XCODE_BUILD_DIR}/${OSX_XCODE_PROJ}
	xcodebuild -target ${OSX_TARGET} -project ${OSX_XCODE_BUILD_DIR}/${OSX_XCODE_PROJ}

${OSX_XCODE_BUILD_DIR}/${OSX_XCODE_PROJ}: cmake-xcode

cmake-xcode:
	@mkdir -p ${OSX_XCODE_BUILD_DIR}
	@cd ${OSX_XCODE_BUILD_DIR} && \
	cmake ../.. ${DARWIN_XCODE_CMAKE_PARAMS}

cmake-osx:
	@mkdir -p ${OSX_BUILD_DIR}
	@cd ${OSX_BUILD_DIR} && \
	cmake ../.. ${DARWIN_CMAKE_PARAMS}

ios: ${IOS_BUILD_DIR}/${IOS_XCODE_PROJ}
	xcodebuild -target ${IOS_TARGET} -project ${IOS_BUILD_DIR}/${IOS_XCODE_PROJ}

${IOS_BUILD_DIR}/${IOS_XCODE_PROJ}: cmake-ios

cmake-ios:
	@mkdir -p ${IOS_BUILD_DIR}
	@cd ${IOS_BUILD_DIR} && \
	cmake ../.. ${IOS_CMAKE_PARAMS}

ios-sim: ${IOS_SIM_BUILD_DIR}/${IOS_XCODE_PROJ}
	xcodebuild -target ${IOS_TARGET} -project ${IOS_SIM_BUILD_DIR}/${IOS_XCODE_PROJ}

${IOS_SIM_BUILD_DIR}/${IOS_XCODE_PROJ}: cmake-ios-sim

cmake-ios-sim:
	@mkdir -p ${IOS_SIM_BUILD_DIR}
	@cd ${IOS_SIM_BUILD_DIR} && \
	cmake ../.. ${IOS_CMAKE_PARAMS} -DIOS_PLATFORM=SIMULATOR

rpi: cmake-rpi
	@cd ${RPI_BUILD_DIR} && \
	${MAKE}

cmake-rpi:
	@mkdir -p ${RPI_BUILD_DIR}
	@cd ${RPI_BUILD_DIR} && \
	cmake ../.. ${RPI_CMAKE_PARAMS}

linux: cmake-linux
	cd ${LINUX_BUILD_DIR} && \
	${MAKE}

cmake-linux:
	mkdir -p ${LINUX_BUILD_DIR}
	cd ${LINUX_BUILD_DIR} &&\
	cmake ../.. ${LINUX_CMAKE_PARAMS}

tests: unit-tests

unit-tests:
	@mkdir -p ${UNIT_TESTS_BUILD_DIR}
	@cd ${UNIT_TESTS_BUILD_DIR} && \
	cmake ../../.. ${UNIT_TESTS_CMAKE_PARAMS} && \
	${MAKE}

check-ndk:
ifndef ANDROID_NDK
	$(error ANDROID_NDK is undefined)
endif

