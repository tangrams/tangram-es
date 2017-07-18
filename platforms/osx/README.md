Mac OS X
========

## Setup ##

This project uses CMake (minimum version 3.0), you can download it [here](http://www.cmake.org/download/) or use your favorite installation package tool like [homebrew](http://brew.sh/).

```bash
brew install cmake
```

Make sure to update git submodules **before** you build with Make:

```bash
git submodule update --init
```

The demo application uses the Mapzen vector tile service, so you will need a Mapzen API key to build and run the demo. 

 1. Visit https://mapzen.com/documentation/overview/#get-started-developing-with-mapzen to get an API key.

 2. Setup an environment variable (`MAPZEN_API_KEY`) to point to your API key. 
 If you are using an IDE on osx, you need to do the following:
    ```bash
    launchctl setenv MAPZEN_API_KEY YOUR_API_KEY
    ```
 If you are running the app from a terminal you need to do the following:
    ```bash
    export MAPZEN_API_KEY=YOUR_API_KEY
    ```

## Build ##

There are several ways you can build the tangram-es library and demo application on Mac OS X:

### Command Line ###

To build a runnable OS X application bundle, run:

```bash
make osx
```

You can optionally use `make -j` to parallelize the build and append `DEBUG=1` or `RELEASE=1` to choose the build type.

Then open the application with:

```bash
open build/osx/bin/tangram.app
```

To open the application with console logs:

```bash
open build/osx/bin/tangram.app/Contents/MacOS/tangram
```

You can open a different YAML scene file by dragging and dropping it into the window, or passing it as an argument:

```bash
open build/osx/bin/tangram.app/Contents/MacOS/tangram -f /path/to/your/scene.yaml
```

### Xcode ###

For running on OS X from Xcode you will need Xcode version 6.0 or higher. Generate and compile an Xcode project:

```bash
make xcode
```

Then just open the Xcode project and run/debug from there:

```bash
open build/xcode/tangram.xcodeproj
```

Note that any Xcode configuration change you make to the project won't be preserved when CMake runs again. Build configuration is defined only in the CMakeLists file(s).

### CLion ###

You can also run and debug from CLion.

After cloning and updating your git submodules, open CLion and __Import Project from Sources__. Select the root of this repo. Choose __Open Project__. Do not overwrite CMakeLists.txt.

CLion will automatically set everything up, all you have to do is wait a minute for the project to get initialized. Then, select the 'tangram' run configuration and hit run/debug.

![CLion Tangram Target](../../images/clion-tangram-target.png)
