Tangram ES (Android) Release Checklist
======================================

Steps to build and release the Tangram ES (AAR) to Maven Central:

## 1. Remove `-SNAPSHOT` tag from the version name in `gradle.properties`. Commit and push to master.
```
GROUP=com.mapzen.tangram
VERSION_NAME=1.0.0
...
```

## 2. Tag commit with release version (ex. tangram-1.0.0) and push tag to GitHub.
```
$ git tag tangram-1.0.0
$ git push origin tangram-1.0.0
```

## 3. Update version name and restore `-SNAPSHOT` tag to prepare next development cycle. Commit and push to master.
```
GROUP=com.mapzen.tangram
VERSION_NAME=1.0.1-SNAPSHOT
...
```

## 4. Once (Travis CI)[1] completes the release build, log into (Sonatype Staging Repository)[2] and promote artifact to production.

**Note: It can may several hours for a newly promoted artifact to appear in (Maven Central)[3].**

## 5. Document release notes at (https://github.com/tangrams/tangram-es/releases)[4].

[1] https://travis-ci.org/tangrams/tangram-es
[2] https://oss.sonatype.org/#stagingRepositories
[3] http://search.maven.org/#search%7Cga%7C1%7Ctangram
[4] https://github.com/tangrams/tangram-es/releases
