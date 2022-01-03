# Tangram ES Release Checklist

## Requirements
- Have cocoapods installed
- Have ownership privileges to update the cocoapods trunk spec
- Have credentials for Sonatype

Steps to release the Tangram ES iOS Framework to Cocoapods and release the Android AAR to Maven Central:

### 1. Prepare release commit
For the Android library, remove `-SNAPSHOT` from the version name in `platforms/android/tangram/gradle.properties`.
```
GROUP=com.mapzen.tangram
VERSION_NAME=1.0.0
```

For the iOS framework, remove `-dev` from the version name in `platforms/ios/config.cmake`:
```
set(FRAMEWORK_VERSION "1.0.0")
```
And increment the version number in `Tangram-es.podspec`:
```
s.version = '1.0.0'
```

For desktop apps, update the version number in `core/include/tangram.h`
```
#define TANGRAM_VERSION_MAJOR 1
#define TANGRAM_VERSION_MINOR 0
#define TANGRAM_VERSION_PATCH 0
```

Commit and push to master.
```
$ git commit -m "Release 1.0.0"
$ git push
```

### 2. Tag release commit
Tag the commit with the release version and push the tag to GitHub.
```
$ git tag 1.0.0
$ git push origin 1.0.0
```

### 3. Create a Release on GitHub
Draft a release for the new tag at [https://github.com/tangrams/tangram-es/releases][5]. 

Document changes in the release notes.

Once the iOS Release build completes on [GitHub Actions][6], download the artifact named `tangram-ios-<git revision>` containing the CocoaPod contents and then attach it to the GitHub release as `tangram-ios-<tag>.zip`. 

Once the Android build completes on [CircleCI][1], download the artifact named `tangram-android-<git revision>.aar` and then attach it to the GitHub release as `tangram-android-<tag>.aar`.

Publish the new release.

### 3. Push iOS framework to CocoaPods
Run `pod spec lint` in the directory containing `Tangram-es.podspec` to validate the Podspec.

Push the podspec to trunk:
```
pod trunk push Tangram-es.podspec
```

### 4. Promote Android artifact to production
Log into [OSS Sonatype Nexus][2] and release to Central.

For details on this process see the [Sonatype OSS Repository Hosting Guide][3]

The new artifact should appear soon in [Maven Central][4].

### 5. Prepare next development cycle
For the Android library, update the version name and restore `-SNAPSHOT` to prepare the next development cycle.
```
GROUP=com.mapzen.tangram
VERSION_NAME=1.0.1-SNAPSHOT
```

For the iOS framework, update the version name and restore `-dev`:
```
set(FRAMEWORK_VERSION "1.0.1-dev")
```

Commit and push to master.
```
$ git commit -m "Prepare next development cycle"
$ git push
```

[1]: https://app.circleci.com/pipelines/github/tangrams/tangram-es
[2]: https://oss.sonatype.org/
[3]: https://central.sonatype.org/pages/ossrh-guide.html
[4]: https://search.maven.org/artifact/com.mapzen.tangram/tangram
[5]: https://github.com/tangrams/tangram-es/releases
[6]: https://github.com/tangrams/tangram-es/actions/workflows/release.yml
