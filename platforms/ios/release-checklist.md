# Tangram ES (iOS) Release Checklist

## Requirements
- Have Xcode installed
- Have cocoapods installed
- Have ownership privileges to update the cocoapods trunk spec

## Steps
1. Go into the Info.plist located at https://github.com/tangrams/tangram-es/blob/master/ios/framework/Info.plist
and update the version tag for the release you're doing. Submit this as a PR for release.
2. Once #1's PR merges, tag that commit with the same version.
3. Build the iOS universal framework for release ( `make clean  && make ios-framework-universal RELEASE=1` ).
4. Take the *universal* framework file and copy it into a branch on the framework holding repo
(https://github.com/tangrams/ios-framework). Submit this for PR following the convention seen in other
PRs for release notes ( https://github.com/tangrams/ios-framework/pull/17 )
5. Once #5's PR merges to master, tag the release with the same version number as #2.
6. Update the Tangram-es.podspec file version number with that same tag, and run `pod spec lint` to
make sure everything is happy. Fix issues if not happy (eventually we should document known possible
issues). Submit to PR once lint is clean.
7. Once #6 PR merges, push the updated pod spec to trunk: `pod trunk push Tangram-es.podspec`
8. Submit a PR to increment the version number for #1's plist file. We add `-dev` to the end of the
version numbers while in development.
9. Before publishing the Tangram release, add the zipped version of both Debug and Release flavors
to the Download attachments of the release.
