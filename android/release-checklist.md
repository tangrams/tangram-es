# Tangram ES (Android) Release Checklist

Steps to build and release the Tangram ES (AAR) to Maven Central:

### 1. Prepare release commit
Remove `-SNAPSHOT` tag from the version name in `gradle.properties`. Commit and push to master.
```
GROUP=com.mapzen.tangram
VERSION_NAME=1.0.0
```

### 2. Tag release commit
Tag commit with release version and push tag to GitHub.
```
$ git tag tangram-1.0.0
$ git push origin tangram-1.0.0
```

### 3. Prepare next development cycle
Update version name and restore `-SNAPSHOT` tag to prepare next development cycle. Commit and push to master.
```
GROUP=com.mapzen.tangram
VERSION_NAME=1.0.1-SNAPSHOT
```

### 4. Promote artifact to production
Once [Travis CI][1] completes the release build, log into [Sonatype Staging Repository][2] and promote the artifact to production. For more information see the [Sonatype OSSRH Guide][3].

**Note: It can several hours for a newly promoted artifact to appear in [Maven Central][4].**

### 5. Document release
Document release notes at [https://github.com/tangrams/tangram-es/releases][5].

[1]: https://travis-ci.org/tangrams/tangram-es
[2]: https://oss.sonatype.org/#stagingRepositories
[3]: http://central.sonatype.org/pages/ossrh-guide.html
[4]: http://search.maven.org/#search%7Cga%7C1%7Ctangram
[5]: https://github.com/tangrams/tangram-es/releases
