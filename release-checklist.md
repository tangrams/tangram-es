# Tangram ES Release Checklist

## Requirements
- Have cocoapods installed
- Have ownership privileges to update the cocoapods trunk spec
- Have credentials for Bintray

Steps to release the Tangram ES iOS Framework to Cocoapods and release the Android AAR to JCenter:

### 1. Prepare release commit
For the Android library, remove `-SNAPSHOT` from the version name in `gradle.properties`.
```
GROUP=com.mapzen.tangram
VERSION_NAME=1.0.0
```

For the iOS framework, remove `-dev` from the version name in `iOS.framework.cmake`:
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

### 3. Push iOS framework to CocoaPods
Once [CircleCI][1] completes the tag build, run `pod spec lint` in the directory containing `Tangram-es.podspec` to validate the Podspec.

Push the podspec to trunk:
```
pod trunk push Tangram-es.podspec
```

### 4. Promote Android artifact to production
Once [CircleCI][1] completes the release build, log into [OSS Sonatype Nexus][2] and release to Central.

For details on this process see the [Sonatype OSS Repository Hosting Guide][3]

The new artifact should appear soon in [Maven Central][4].

### 5. Prepare next development cycle
For the Android library, update the version name and restore `-SNAPSHOT` to prepare the next development cycle.
```
GROUP=com.mapzen.tangram
VERSION_NAME=1.0.1-SNAPSHOT
```

For the iOS framework, update the version name and restore `-dev` in `iOS.framework.cmake`:
```
set(FRAMEWORK_VERSION "1.0.1-dev")
```

Commit and push to master.
```
$ git commit -m "Prepare next development cycle"
$ git push
```

### 6. Document release
Document release notes at [https://github.com/tangrams/tangram-es/releases][5].

[1]: https://app.circleci.com/pipelines/github/tangrams/tangram-es
[2]: https://oss.sonatype.org/
[3]: https://central.sonatype.org/pages/ossrh-guide.html
[4]: https://search.maven.org/artifact/com.mapzen.tangram/tangram
[5]: https://github.com/tangrams/tangram-es/releases
