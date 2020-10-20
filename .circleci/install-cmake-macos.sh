# Get version number from shell variable or a default.
CMAKE_VERSION=${CMAKE_VERSION-3.18.4}
# Download and unpack the CMake release.
curl --location https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Darwin-x86_64.tar.gz | tar -xz
# Create a link for the CMake binaries on the user $PATH.
for file in cmake-${CMAKE_VERSION}-Darwin-x86_64/CMake.app/Contents/bin/*
do
	ln -s ${file} /usr/local/bin/$(basename ${file})
done
# Check that the shell finds the CMake binary.
./cmake --version
