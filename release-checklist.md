# Tangram ES Release Checklist

## Requirements
- Have cocoapods installed
- Have ownership privileges to update the cocoapods trunk spec
- Have credentials for Sonatype

Steps to release the Tangram ES iOS Framework to Cocoapods and release the Android AAR to Maven Central:

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

### 3. Prepare next development cycle
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

### 4. Push iOS framework to CocoaPods
Once [CircleCI][1] completes the tag build, run `pod spec lint` in the directory containing `Tangram-es.podspec` to validate the Podspec.

Push the podspec to trunk:
```
pod trunk push Tangram-es.podspec
```

### 5. Promote Android artifact to production
Once [Travis CI][2] completes the release build, log into [Sonatype Staging Repository][3] and promote the artifact to production. For more information see the [Sonatype OSSRH Guide][4].

**Note: It can several hours for a newly promoted artifact to appear in [Maven Central][5].**

### 6. Document release
Document release notes at [https://github.com/tangrams/tangram-es/releases][6].

[1]: https://circleci.com/gh/tangrams/tangram-es
[2]: https://travis-ci.org/tangrams/tangram-es
[3]: https://oss.sonatype.org/#stagingRepositories
[4]: http://central.sonatype.org/pages/ossrh-guide.html
[5]: http://search.maven.org/#search%7Cga%7C1%7Ctangram
[6]: https://github.com/tangrams/tangram-es/releases
