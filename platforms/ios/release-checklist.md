# Tangram ES (iOS) Release Checklist

## Requirements
- Have Xcode installed
- Have cocoapods installed
- Have ownership privileges to update the cocoapods trunk spec

Steps to release the Tangram ES iOS Framework to Cocoapods:

### 1. Prepare release commit
Remove `-dev` from the version number in `iOS.framework.cmake`:
```
set(FRAMEWORK_VERSION "1.0.0")
```
And increment the version number in `Tangram-es.podspec`:
```
s.version = '1.0.0'
```
Commit and push to master.

### 2. Tag release commit
Tag the commit with the release version and push the tag to GitHub.
```
$ git tag 1.0.0
$ git push origin 1.0.0
```

### 3. Prepare next development cycle
Increment the version and restore `-dev` to the version number in `iOS.framework.cmake`:
```
set(FRAMEWORK_VERSION "1.0.1-dev")
```
Commit and push to master.

### 4. Push framework to CocoaPods
Once [CircleCI](https://circleci.com/gh/tangrams/tangram-es) completes the tag build, run `pod spec lint` in the directory containing `Tangram-es.podspec` to validate the Podspec.

Push the podspec to trunk:
```
pod trunk push Tangram-es.podspec
```

### 5. Document release
Document release notes at https://github.com/tangrams/tangram-es/releases.
