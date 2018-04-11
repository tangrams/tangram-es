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
Once [Travis CI][2] completes the release build, log into [Bintray][3] and publish the new artifacts to production.

The new artifact should appear soon in [JCenter][4].

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

[1]: https://circleci.com/gh/tangrams/tangram-es
[2]: https://travis-ci.org/tangrams/tangram-es
[3]: https://bintray.com/tangrams/maven/tangram
[4]: https://bintray.com/bintray/jcenter
[5]: https://github.com/tangrams/tangram-es/releases
