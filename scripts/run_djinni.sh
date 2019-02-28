#!/bin/bash -eo pipefail

if [[ -z $DJINNI ]]; then
	echo "Set DJINNI to your djinni repository path and try again."
	exit 1
fi

in="$1"
cpp_out="core/src"
hpp_out="core/include/tangram"
jni_out="platforms/android/tangram/src/main/cpp"
java_out="platforms/android/tangram/src/main/java/com/mapzen/tangram"
objc_out="platforms/ios/framework/src"

java_package="com.mapzen.tangram"

# Build djinni.
"$DJINNI/src/build"

# Choose temporary output location.
tmp_out="build/djinni-tmp-out"

# Clean temporary output folder.
[ ! -e "$tmp_out" ] || rm -r "$tmp_out"

# Run the code generator.
"$DJINNI/src/run-assume-built" \
	--java-out "$tmp_out/java" \
	--java-package "$java-package" \
	--java-class-access-modifier package \
	--java-generate-interfaces true \
	--ident-java-field mFooBar \
	\
	--cpp-out "$tmp_out/cpp" \
	--cpp-namespace tangram \
	--ident-cpp-file FooBar \
	--ident-cpp-enum-type FooBar \
	--ident-cpp-enum FOO_BAR \
	\
	--jni-out "$tmp_out/jni" \
	--ident-jni-class NativeFooBar \
	--ident-jni-file NativeFooBar \
	\
	--objc-out "$tmp_out/objc" \
	--objcpp-out "$tmp_out/objc" \
	--objc-type-prefix TG \
	--objc-swift-bridging-header Tangram-BridgingHeader \
	\
	--idl "$in"

# Copy files from temporary output to final location.
mirror() {
    local prefix="$1" ; shift
    local src="$1" ; shift
    local dest="$1" ; shift
    mkdir -p "$dest"
    rsync -r --checksum --itemize-changes "$src"/ "$dest" | sed "s/^/[$prefix]/"
}

mirror "cpp" "$tmp_out/cpp" "$cpp_out"
mirror "java" "$tmp_out/java" "$java_out"
mirror "jni" "$tmp_out/jni" "$jni_out"
mirror "objc" "$tmp_out/objc" "$objc_out"

echo "djinni completed."
