all: android osx ios

.PHONY: clean
.PHONY: clean-android
.PHONY: clean-osx
.PHONY: clean-xcode
.PHONY: clean-ios
.PHONY: clean-rpi
.PHONY: clean-linux
.PHONY: clean-benchmark
.PHONY: clean-shaders
.PHONY: clean-tizen-arm
.PHONY: clean-tizen-x86
.PHONY: clean-ios-framework
.PHONY: clean-ios-framework-sim
.PHONY: android
.PHONY: osx
.PHONY: xcode
.PHONY: ios
.PHONY: rpi
.PHONY: linux
.PHONY: benchmark
.PHONY: ios-framework
.PHONY: ios-framework-universal
.PHONY: check-ndk
.PHONY: cmake-osx
.PHONY: cmake-xcode
.PHONY: cmake-ios
.PHONY: cmake-ios-framework
.PHONY: cmake-ios-framework-sim
.PHONY: cmake-rpi
.PHONY: cmake-linux
.PHONY: install-android
.PHONY: ios-docs

ANDROID_BUILD_DIR = platforms/android/tangram/build
OSX_BUILD_DIR = build/osx
OSX_XCODE_BUILD_DIR = build/xcode
IOS_BUILD_DIR = build/ios
IOS_FRAMEWORK_BUILD_DIR = build/ios-framework
IOS_FRAMEWORK_SIM_BUILD_DIR = build/ios-framework-sim
IOS_FRAMEWORK_UNIVERSAL_BUILD_DIR = build/ios-framework-universal
IOS_SIM_BUILD_DIR = build/ios-sim
IOS_DOCS_BUILD_DIR = build/ios-docs
RPI_BUILD_DIR = build/rpi
LINUX_BUILD_DIR = build/linux
TESTS_BUILD_DIR = build/tests
BENCH_BUILD_DIR = build/bench
TIZEN_ARM_BUILD_DIR = build/tizen-arm
TIZEN_X86_BUILD_DIR = build/tizen-x86

TOOLCHAIN_DIR = toolchains
OSX_TARGET = tangram
IOS_TARGET = tangram
IOS_FRAMEWORK_TARGET = TangramMap
OSX_XCODE_PROJ = tangram.xcodeproj
IOS_XCODE_PROJ = tangram.xcodeproj
IOS_FRAMEWORK_XCODE_PROJ = tangram.xcodeproj

XCPRETTY = $(shell command -v xcpretty 2> /dev/null)

# Default build type is Release
CONFIG = Release
ifdef DEBUG
	CONFIG = Debug
endif

ifdef DEBUG
	BUILD_TYPE = -DCMAKE_BUILD_TYPE=Debug
endif
ifdef RELEASE
	BUILD_TYPE = -DCMAKE_BUILD_TYPE=Release
endif

ifndef TANGRAM_CMAKE_OPTIONS
	TANGRAM_CMAKE_OPTIONS = \
		-DBENCHMARK=0 \
		-DUNIT_TESTS=0
endif

# Build for iOS simulator architecture only
ifdef TANGRAM_IOS_FRAMEWORK_SLIM
	IOS_FRAMEWORK_PATH = ${IOS_FRAMEWORK_SIM_BUILD_DIR}/lib/${CONFIG}/TangramMap.framework
	IOS_FRAMEWORK_DEVICE_ARCHS = ''
	IOS_FRAMEWORK_SIM_ARCHS = 'x86_64'
else
	IOS_FRAMEWORK_PATH = ${IOS_FRAMEWORK_UNIVERSAL_BUILD_DIR}/${CONFIG}/TangramMap.framework
	IOS_FRAMEWORK_DEVICE_ARCHS = 'armv7 armv7s arm64'
	IOS_FRAMEWORK_SIM_ARCHS = 'i386 x86_64'
endif

BENCH_CMAKE_PARAMS = \
	-DBENCHMARK=1 \
	-DAPPLICATION=0 \
	-DCMAKE_BUILD_TYPE=Release

UNIT_TESTS_CMAKE_PARAMS = \
	-DUNIT_TESTS=1 \
	-DAPPLICATION=0 \
	-DCMAKE_BUILD_TYPE=Debug

IOS_CMAKE_PARAMS = \
        ${BUILD_TYPE} \
        ${CMAKE_OPTIONS} \
	-DPLATFORM_TARGET=ios \
	-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_DIR}/iOS.toolchain.cmake \
	-DTANGRAM_FRAMEWORK=${IOS_FRAMEWORK_PATH} \
	-G Xcode

IOS_FRAMEWORK_CMAKE_PARAMS = \
        ${BUILD_TYPE} \
        ${CMAKE_OPTIONS} \
	-DPLATFORM_TARGET=ios.framework \
	-DIOS_SIMULATOR_ARCHS=${IOS_FRAMEWORK_SIM_ARCHS} \
	-DIOS_DEVICE_ARCHS=${IOS_FRAMEWORK_DEVICE_ARCHS} \
	-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_DIR}/iOS.toolchain.cmake \
	-G Xcode

DARWIN_XCODE_CMAKE_PARAMS = \
        ${BUILD_TYPE} \
        ${CMAKE_OPTIONS} \
	-DPLATFORM_TARGET=darwin \
	-DCMAKE_OSX_DEPLOYMENT_TARGET:STRING="10.9" \
	-G Xcode

DARWIN_CMAKE_PARAMS = \
        ${BUILD_TYPE} \
        ${CMAKE_OPTIONS} \
	-DPLATFORM_TARGET=darwin \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE

RPI_CMAKE_PARAMS = \
        ${BUILD_TYPE} \
        ${CMAKE_OPTIONS} \
	-DPLATFORM_TARGET=raspberrypi \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE

LINUX_CMAKE_PARAMS = \
        ${BUILD_TYPE} \
        ${CMAKE_OPTIONS} \
	-DPLATFORM_TARGET=linux \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE

ifndef TIZEN_PROFILE
	TIZEN_PROFILE=mobile
endif

ifndef TIZEN_VERSION
	TIZEN_VERSION=3.0
endif

TIZEN_ARM_CMAKE_PARAMS = \
        ${BUILD_TYPE} \
        ${CMAKE_OPTIONS} \
	-DTIZEN_SDK=$$TIZEN_SDK \
	-DTIZEN_SYSROOT=$$TIZEN_SDK/platforms/tizen-${TIZEN_VERSION}/${TIZEN_PROFILE}/rootstraps/${TIZEN_PROFILE}-${TIZEN_VERSION}-device.core \
	-DTIZEN_DEVICE=1 \
	-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_DIR}/tizen.toolchain.cmake \
	-DPLATFORM_TARGET=tizen \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE

TIZEN_X86_CMAKE_PARAMS = \
	${BUILD_TYPE} \
	${CMAKE_OPTIONS} \
	-DTIZEN_SDK=$$TIZEN_SDK \
	-DTIZEN_SYSROOT=$$TIZEN_SDK/platforms/tizen-${TIZEN_VERSION}/${TIZEN_PROFILE}/rootstraps/${TIZEN_PROFILE}-${TIZEN_VERSION}-emulator.core \
	-DTIZEN_DEVICE=0 \
	-DCMAKE_TOOLCHAIN_FILE=${TOOLCHAIN_DIR}/tizen.toolchain.cmake \
	-DPLATFORM_TARGET=tizen \
	-DCMAKE_EXPORT_COMPILE_COMMANDS=TRUE

clean: clean-android clean-osx clean-ios clean-rpi clean-tests clean-xcode clean-linux clean-shaders \
	clean-tizen-arm clean-tizen-x86

clean-android:
	rm -rf android/build
	rm -rf android/tangram/build
	rm -rf android/tangram/.externalNativeBuild
	rm -rf android/demo/build

clean-osx:
	rm -rf ${OSX_BUILD_DIR}

clean-ios: clean-ios-framework clean-ios-framework-sim clean-ios-framework-universal
	rm -rf ${IOS_BUILD_DIR}

clean-rpi:
	rm -rf ${RPI_BUILD_DIR}

clean-linux:
	rm -rf ${LINUX_BUILD_DIR}

clean-xcode:
	rm -rf ${OSX_XCODE_BUILD_DIR}

clean-tests:
	rm -rf ${TESTS_BUILD_DIR}

clean-benchmark:
	rm -rf ${BENCH_BUILD_DIR}

clean-shaders:
	rm -rf core/include/shaders/*.h

clean-tizen-arm:
	rm -rf ${TIZEN_ARM_BUILD_DIR}

clean-tizen-x86:
	rm -rf ${TIZEN_X86_BUILD_DIR}

clean-ios-framework:
	rm -rf ${IOS_FRAMEWORK_BUILD_DIR}

clean-ios-framework-sim:
	rm -rf ${IOS_FRAMEWORK_SIM_BUILD_DIR}

clean-ios-framework-universal:
	rm -rf ${IOS_FRAMEWORK_UNIVERSAL_BUILD_DIR}

android: android-demo
	@echo "run: 'adb install -r android/demo/build/outputs/apk/demo-debug.apk'"

android-sdk:
	@cd platforms/android/ && \
	./gradlew tangram:assembleFullRelease

android-demo:
	@cd platforms/android/ && \
	./gradlew demo:assembleDebug

osx: ${OSX_BUILD_DIR}/Makefile
	@cd ${OSX_BUILD_DIR} && \
	${MAKE}

${OSX_BUILD_DIR}/Makefile: cmake-osx

OSX_BUILD = \
	xcodebuild -target ${OSX_TARGET} -project ${OSX_XCODE_BUILD_DIR}/${OSX_XCODE_PROJ} -configuration ${CONFIG}

xcode: ${OSX_XCODE_BUILD_DIR}/${OSX_XCODE_PROJ}
ifeq (, $(shell which xcpretty))
	${OSX_BUILD}
else
	${OSX_BUILD} | ${XCPRETTY}
endif

${OSX_XCODE_BUILD_DIR}/${OSX_XCODE_PROJ}: cmake-xcode

cmake-xcode:
	@mkdir -p ${OSX_XCODE_BUILD_DIR}
	@cd ${OSX_XCODE_BUILD_DIR} && \
	cmake ../.. ${DARWIN_XCODE_CMAKE_PARAMS}

cmake-osx:
	@mkdir -p ${OSX_BUILD_DIR}
	@cd ${OSX_BUILD_DIR} && \
	cmake ../.. ${DARWIN_CMAKE_PARAMS}

IOS_BUILD = \
	xcodebuild -target ${IOS_TARGET} ARCHS=${IOS_FRAMEWORK_SIM_ARCHS} ONLY_ACTIVE_ARCH=NO -sdk iphonesimulator -project ${IOS_BUILD_DIR}/${IOS_XCODE_PROJ} -configuration ${CONFIG}

ios: ${IOS_BUILD_DIR}/${IOS_XCODE_PROJ}
ifeq (, $(shell which xcpretty))
	${IOS_BUILD}
else
	${IOS_BUILD} | ${XCPRETTY}
endif

ios-docs:
ifeq (, $(shell which jazzy))
	$(error "Please install jazzy by running 'gem install jazzy'")
endif
	@mkdir -p ${IOS_DOCS_BUILD_DIR}
	@cd platforms/ios && \
	jazzy --config jazzy.yml

${IOS_BUILD_DIR}/${IOS_XCODE_PROJ}: cmake-ios

cmake-ios: ios-framework-universal
	@mkdir -p ${IOS_BUILD_DIR}
	@cd ${IOS_BUILD_DIR} && \
	cmake ../.. ${IOS_CMAKE_PARAMS}

cmake-ios-framework:
ifndef TANGRAM_IOS_FRAMEWORK_SLIM
	@mkdir -p ${IOS_FRAMEWORK_BUILD_DIR}
	@cd ${IOS_FRAMEWORK_BUILD_DIR} && \
	cmake ../.. ${IOS_FRAMEWORK_CMAKE_PARAMS} -DBUILD_IOS_FRAMEWORK=TRUE
endif

cmake-ios-framework-sim:
	@mkdir -p ${IOS_FRAMEWORK_SIM_BUILD_DIR}
	@cd ${IOS_FRAMEWORK_SIM_BUILD_DIR} && \
	cmake ../.. ${IOS_FRAMEWORK_CMAKE_PARAMS} -DIOS_PLATFORM=SIMULATOR -DBUILD_IOS_FRAMEWORK=TRUE

IOS_FRAMEWORK_BUILD = \
	xcodebuild -target ${IOS_FRAMEWORK_TARGET} -project ${IOS_FRAMEWORK_BUILD_DIR}/${IOS_FRAMEWORK_XCODE_PROJ} -configuration ${CONFIG}

ios-framework: cmake-ios-framework
ifndef TANGRAM_IOS_FRAMEWORK_SLIM
ifeq (, $(shell which xcpretty))
	${IOS_FRAMEWORK_BUILD}
else
	${IOS_FRAMEWORK_BUILD} | ${XCPRETTY}
endif
endif

IOS_FRAMEWORK_SIM_BUILD = \
	xcodebuild -target ${IOS_FRAMEWORK_TARGET} -project ${IOS_FRAMEWORK_SIM_BUILD_DIR}/${IOS_FRAMEWORK_XCODE_PROJ} -configuration ${CONFIG}

ios-framework-sim: cmake-ios-framework-sim
ifeq (, $(shell which xcpretty))
	${IOS_FRAMEWORK_SIM_BUILD}
else
	${IOS_FRAMEWORK_SIM_BUILD} | ${XCPRETTY}
endif

ios-framework-universal: ios-framework ios-framework-sim
ifndef TANGRAM_IOS_FRAMEWORK_SLIM
	@mkdir -p ${IOS_FRAMEWORK_UNIVERSAL_BUILD_DIR}/${CONFIG}
	@cp -r ${IOS_FRAMEWORK_BUILD_DIR}/lib/${CONFIG}/TangramMap.framework ${IOS_FRAMEWORK_UNIVERSAL_BUILD_DIR}/${CONFIG}
	lipo ${IOS_FRAMEWORK_BUILD_DIR}/lib/${CONFIG}/TangramMap.framework/TangramMap \
		${IOS_FRAMEWORK_SIM_BUILD_DIR}/lib/${CONFIG}/TangramMap.framework/TangramMap \
		-create -output ${IOS_FRAMEWORK_UNIVERSAL_BUILD_DIR}/${CONFIG}/TangramMap.framework/TangramMap
endif

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

tizen-arm: cmake-tizen-arm
	cd ${TIZEN_ARM_BUILD_DIR} && \
	${MAKE}

cmake-tizen-arm:
	mkdir -p ${TIZEN_ARM_BUILD_DIR}
	cd ${TIZEN_ARM_BUILD_DIR} &&\
	cmake ../.. ${TIZEN_ARM_CMAKE_PARAMS}

tizen-x86: cmake-tizen-x86
	cd ${TIZEN_X86_BUILD_DIR} && \
	${MAKE}

cmake-tizen-x86:
	mkdir -p ${TIZEN_X86_BUILD_DIR}
	cd ${TIZEN_X86_BUILD_DIR} && \
	cmake ../.. ${TIZEN_X86_CMAKE_PARAMS}

tests: unit-tests

unit-tests:
	@mkdir -p ${TESTS_BUILD_DIR}
	@cd ${TESTS_BUILD_DIR} && \
	cmake ../.. ${UNIT_TESTS_CMAKE_PARAMS} && \
	${MAKE}

benchmark:
	@mkdir -p ${BENCH_BUILD_DIR}
	@cd ${BENCH_BUILD_DIR} && \
	cmake ../../ ${BENCH_CMAKE_PARAMS} && \
	${MAKE}

format:
	@for file in `git diff --diff-filter=ACMRTUXB --name-only -- '*.cpp' '*.h'`; do \
		if [[ -e $$file ]]; then clang-format -i $$file; fi \
	done
	@echo "format done on `git diff --diff-filter=ACMRTUXB --name-only -- '*.cpp' '*.h'`"

### Android Helpers
android-install:
	@adb install -r platforms/android/demo/build/outputs/apk/demo-debug.apk
