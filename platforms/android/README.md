Android
=======
[ ![Download](https://api.bintray.com/packages/tangrams/maven/tangram/images/download.svg) ](https://bintray.com/tangrams/maven/tangram/_latestVersion)

The recommended way use tangram-es in an Android project is to add it as a Gradle dependency. The library is hosted on jcenter in the package 'com.mapzen.tangram'. To see all available versions, check [Bintray](https://bintray.com/tangrams/maven/tangram).

If jcenter is not set up as a dependency repository, add it in your project's 'build.gradle' file:

```
allprojects {
  dependencies {
    repositories {
      jcenter()
    }
  }
}
```

Then add tangram-es in the 'dependencies' section of your module's 'build.gradle' file:

```
dependencies {
  compile 'com.mapzen.tangram:tangram:$latest_version'
}
```

That's it! If you want to build tangram-es for Android from scratch, continue reading.

## Setup ##

To build for Android you'll need [Android Studio](https://developer.android.com/studio/index.html) version 3.0 or newer on Mac OS X, Ubuntu, or Windows 10. Using the Android Studio SDK Manager, install or update the 'CMake', 'LLDB', and 'NDK' packages from the 'SDK Tools' tab.

The demo application uses the Nextzen vector tile service, so you will need a Nextzen API key to build and run the demo. 

 1. Visit https://developers.nextzen.org/ to get an API key.

 2. In your local Gradle properties file (`~/.gradle/gradle.properties`) add the following line, substituting your API key: `nextzenApiKey=yourApiKeyHere`

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
