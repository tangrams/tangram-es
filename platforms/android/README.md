Android
=======

The recommended way use tangram-es in an Android project is to add it as a Gradle dependency. The library is hosted on Maven Central in the package 'com.mapzen.tangram'. To find the latest version number, check [Maven Central](http://search.maven.org/#search%7Cga%7C1%7Cg%3A%22com.mapzen.tangram%22).

If Maven Central is not set up as a dependency repository, add it in your project's 'build.gradle' file:

```
allprojects {
  dependencies {
    repositories {
      mavenCentral()
    }
  }
}
```

Then add tangram-es in the 'dependencies' section of your module's 'build.gradle' file:

```
dependencies {
  compile 'com.mapzen.tangram:tangram:0.5.0'
}
```

That's it! If you want to build tangram-es for Android from scratch, continue reading.

## Setup ##

To build for Android you'll need [Android Studio](https://developer.android.com/studio/index.html) version 2.2 or newer on Mac OS X, Ubuntu, or Windows 10. Using the Android Studio SDK Manager, install or update the 'CMake', 'LLDB', and 'NDK' packages from the 'SDK Tools' tab.

## Build ##

After installing dependencies in Android Studio, you can execute Android builds from either the command line or the Android Studio interface.

### Command Line ###

The Android project is executed with Gradle commands from the 'platforms/android' folder. To build the demo application for the ARMv7 architecture (covers most Android devices), run:

```bash
./gradlew demo:assembleDebug
```

To install the demo on a connected Android device, run:

```bash
./gradlew demo:installDebug
```

### Android Studio ###

Open the project in Android Studio, select 'demo' from the Configurations menu, then press the 'Run' button (^R).

Android Studio supports debugging both the Java and C++ parts of tangram-es on a connected device or emulator. Choose one of the 'debug' build variants, set your desired breakpoints, and press the 'Debug' button (^D).
