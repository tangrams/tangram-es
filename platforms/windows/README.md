# Windows

## Setup

This project uses CMake (minimum version 3.0), you can download it [here](http://www.cmake.org/download/).

Make sure to update git submodules before you build:

```bash
git submodule update --init
```

Currently, builds were tested under MinGW-w64 (recommended). You will need to install/build zlib and curl.

The demo application uses the Nextzen vector tile service, so you will need a Nextzen API key to build and run the demo.

 1. Visit https://developers.nextzen.org/ to get an API key.

 2. Setup an environment variable (`NEXTZEN_API_KEY`) to point to your API key.
    ```bash
    SET NEXTZEN_API_KEY=YOUR_API_KEY
    ```
    Or via system settings.

## Build

  Tangram has been reported work with 32-bit MinGW-w64 consisting of
  GCC 8.1, POSIX threads and SJLJ exceptions. I've experienced crashes because
  of exception handling during YAML parsing when using DWARF2 variant - any
  recent version with SJLJ should work fine.

 1. Download zlib sources from https://zlib.net/

 2. Download curl sources fom https://curl.haxx.se/download.html

### Zlib and Curl under MinGW-w64

- CMake typically installs everything under Program Files, which is unfortunate
  because it needs elevated privileges and also bloats dir quickly. I strongly
  encourage you to make dedicated directory, e.g. `c:/prg/cmake_install`.
  To make it work, set `CMAKE_PREFIX_PATH` env var to this directory.
  If `echo %CMAKE_PREFIX_PATH%` doesn't reflect changes, you must restart
  command prompt or log out and log in to Windows.
- Using latest curl & zlib source packages is usually the best idea.

``` bat
cd /d C:\path\to\zlib-1.2.11
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_INSTALL_PREFIX=c:/prg/cmake_install
mingw32-make
mingw32-make install
```

``` bat
cd /d C:\path\to\curl-7.60.0
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_INSTALL_PREFIX=c:/prg/cmake_install -DCMAKE_USE_WINSSL=ON
mingw32-make
mingw32-make install
```

### Tangram via MinGW

In the root directory type `mingw32-make windows` or manually:

- Create directory build/windows under tangram-es root directory.
- Enter command prompt and go to newly created directory.
- Setup CMake, forcing MinGW toolchain: `cmake ../.. -G "MinGW Makefiles"`
- Run build via make: `mingw32-make`

You can optionally use `mingw32-make -j#` to parallelize the build
and append `DEBUG=1` or `RELEASE=1` to choose the build type.

### Tangram via Clang

** Last time I've checked (LLVM5) Clang wasn't able to build tangram-es because
of some kind of MSVC quirks compatibility. Proceed at your own risk.**

You need to have MS Build tools installed. Proceed as manually with MinGW, but call cmake with following:

`cmake -DCMAKE_C_COMPILER=clang-cl.exe -DCMAKE_CXX_COMPILER=clang-cl.exe -G "NMake Makefiles"  ../..`

My last try ended up with:

``` txt
D:\prg\_git\tangram-es\core\deps\harfbuzz-icu-freetype\icu\common\ucnv2022.cpp(751,9):  error: ISO C++17 does not allow
      'register' storage class specifier [-Wregister]
```

### Tangram via MSVC

** Last time I've checked (vs2015) MSVC wasn't able to build tangram-es.
Proceed at your own risk.**

You need to have MS Build tools installed.

- Launch Visual C++ 2015 x86 Native Build Tools Command Prompt
- Enter zlib directory and run ```nmake -f win32/Makefile.msc```
- Enter `<curl dir>/winbuild` and run `nmake /f Makefile.vc mode=<static or dll>`
- For static builds, go to
  `<curl dir>\builds\libcurl-vc-x86-release-static-ipv6-sspi-winssl\lib`
  and change `libcurl_a.lib` to `libcurl.lib`
- Setup local variable `CMAKE_PREFIX_PATH` so that it includes
  `<curl dir>\builds\libcurl-vc-x86-release-static-ipv6-sspi-winssl`
  and `<zlib dir>`. If `echo %CMAKE_PREFIX_PATH%` doesn't reflect changes,
  you must restart command prompt or log out and log in to Windows.
- Create directory build/windows under tangram-es root directory.
- Enter command prompt and go to newly created directory.
- Setup CMake: `cmake ../.. -G "NMake Makefiles"`
- Run build via nmake: `nmake`

## Running app

App should now reside in `build/windows/bin` directory.
If you built curl/zlib as dynamic libraries, you'll have to supply zlib.dll
and curl.dll into same directory as .exe file.

You can open a different YAML scene file by dragging and dropping it into the
window, or passing it as an argument:

`tangram -f /path/to/your/scene.yaml`
