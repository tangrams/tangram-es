# Tangram ES (iOS) Release Checklist

## Requirements
- Have Xcode installed
- Have cocoapods installed
- Have ownership privileges to update the cocoapods trunk spec

## Steps
1. Update the version tag `${FRAMEWORK_VERSION}` that gets injected through CMake to the .plist file
of the framework for the release you're doing. Submit this as a PR for release.
2. Build the iOS universal framework for release ( `make clean  && make ios-framework-universal RELEASE=1` ).
3. Take the *universal* framework file and copy it into a branch on the framework holding repo
(https://github.com/tangrams/ios-framework). Submit this for PR following the convention seen in other
PRs for release notes ( https://github.com/tangrams/ios-framework/pull/17 )
4. Once #5's PR merges to master, tag the release with the same version number as #2.
5. Update the Tangram-es.podspec file version number with that same tag, and run `pod spec lint` to
make sure everything is happy. Fix issues if not happy (eventually we should document known possible
issues). Submit to PR once lint is clean.
6. Once #6 PR merges, push the updated pod spec to trunk: `pod trunk push Tangram-es.podspec`
7. Submit a PR to increment the version number for #1's plist file. We add `-dev` to the end of the
version numbers while in development.
8. Before publishing the Tangram release, add the zipped version of both Debug and Release flavors
to the Download attachments of the release.
