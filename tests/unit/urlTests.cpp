#include "catch.hpp"

#include "util/url.h"

using namespace Tangram;

TEST_CASE("Parse components of a correctly formatted URL", "[Url]") {

    // Tests conformance to https://tools.ietf.org/html/rfc1808#section-2.1

    Url url("https://some.domain:9000/path/to/file.html;param=val?api_key=mapsRcool#yolo");

    CHECK(!url.isEmpty());
    CHECK(url.isAbsolute());
    CHECK(!url.hasDataScheme());
    CHECK(!url.hasBase64Data());
    CHECK(!url.hasFileScheme());
    CHECK(url.hasHttpScheme());
    CHECK(url.hasScheme());
    CHECK(url.scheme() == "https");
    CHECK(url.hasNetLocation());
    CHECK(url.netLocation() == "some.domain:9000");
    CHECK(url.hasPath());
    CHECK(url.path() == "/path/to/file.html");
    CHECK(url.hasParameters());
    CHECK(url.parameters() == "param=val");
    CHECK(url.hasQuery());
    CHECK(url.query() == "api_key=mapsRcool");
    CHECK(url.hasFragment());
    CHECK(url.fragment() == "yolo");
    CHECK(!url.hasMediaType());
    CHECK(!url.hasData());

}

TEST_CASE("Parse components of a correctly formatted data URI", "[Url]") {

    // Tests conformance to https://tools.ietf.org/html/rfc2397#section-3

    Url url("data:text/html;charset=utf-8;base64,YmFzZTY0");

    CHECK(!url.isEmpty());
    CHECK(url.isAbsolute());
    CHECK(url.hasDataScheme());
    CHECK(url.hasBase64Data());
    CHECK(!url.hasFileScheme());
    CHECK(!url.hasHttpScheme());
    CHECK(url.hasScheme());
    CHECK(url.scheme() == "data");
    CHECK(!url.hasNetLocation());
    CHECK(!url.hasParameters());
    CHECK(!url.hasQuery());
    CHECK(!url.hasFragment());
    CHECK(url.hasMediaType());
    CHECK(url.mediaType() == "text/html;charset=utf-8");
    CHECK(url.hasData());
    CHECK(url.data() == "YmFzZTY0");

}

TEST_CASE("Parse an empty URL", "[Url]") {

    Url url("");

    CHECK(url.isEmpty());
    CHECK(!url.isAbsolute());
    CHECK(!url.hasDataScheme());
    CHECK(!url.hasBase64Data());
    CHECK(!url.hasScheme());
    CHECK(!url.hasNetLocation());
    CHECK(!url.hasPath());
    CHECK(!url.hasParameters());
    CHECK(!url.hasQuery());
    CHECK(!url.hasFragment());
    CHECK(!url.hasMediaType());
    CHECK(!url.hasData());

}

TEST_CASE("Remove dot segments from a path", "[Url]") {

    // Tests conformance to https://tools.ietf.org/html/rfc3986#section-5.2.4

    CHECK(Url::removeDotSegmentsFromString("") == "");
    CHECK(Url::removeDotSegmentsFromString("a/b/c") == "a/b/c");
    CHECK(Url::removeDotSegmentsFromString("a/b=?.;5/c") == "a/b=?.;5/c");
    CHECK(Url::removeDotSegmentsFromString("/a/b/c/./../../g") == "/a/g");
    CHECK(Url::removeDotSegmentsFromString("../a/b") == "a/b");
    CHECK(Url::removeDotSegmentsFromString("./") == "");
    CHECK(Url::removeDotSegmentsFromString("..") == "");
    CHECK(Url::removeDotSegmentsFromString("a/b/../../..") == "");
    CHECK(Url::removeDotSegmentsFromString("a/b/../c/../d/./e/..") == "a/d");
    CHECK(Url::removeDotSegmentsFromString("a//b//c") == "a//b//c");
    CHECK(Url::removeDotSegmentsFromString("a./b../..c/.d") == "a./b../..c/.d");

}

TEST_CASE("Produce a 'standardized' URL", "[Url]") {

    CHECK(Url("http://example.com/path/oops/not/here/../../../file.txt;p?q#f").standardized().string() == "http://example.com/path/file.txt;p?q#f");
    CHECK(Url("http://example.com/../../no/going/back/file.txt;p?q#f").standardized().string() == "http://example.com/no/going/back/file.txt;p?q#f");
    CHECK(Url("data:text/html;charset=utf-8,LoremIpsum").standardized().string() == "data:text/html;charset=utf-8,LoremIpsum");

}

TEST_CASE("Maintain URL components when 'standardized'", "[URL]") {

    Url url(Url("http://mapzen.com/nothing/to/see/here/../../../../index.html;p?q#f").standardized());

    CHECK(!url.isEmpty());
    CHECK(url.isAbsolute());
    CHECK(!url.hasDataScheme());
    CHECK(!url.hasBase64Data());
    CHECK(!url.hasFileScheme());
    CHECK(url.hasHttpScheme());
    CHECK(url.hasScheme());
    CHECK(url.scheme() == "http");
    CHECK(url.hasNetLocation());
    CHECK(url.netLocation() == "mapzen.com");
    CHECK(url.hasPath());
    CHECK(url.path() == "/index.html");
    CHECK(url.hasParameters());
    CHECK(url.parameters() == "p");
    CHECK(url.hasQuery());
    CHECK(url.query() == "q");
    CHECK(url.hasFragment());
    CHECK(url.fragment() == "f");
    CHECK(!url.hasMediaType());
    CHECK(!url.hasData());

}

TEST_CASE("Resolve a URL against an absolute base URL", "[Url]") {

    // https://tools.ietf.org/html/rfc3986#section-5.4.1

    Url base("http://a/b/c/d;p?q");

    CHECK(Url("g:h").resolved(base).string() == "g:h");
    CHECK(Url("g").resolved(base).string() == "http://a/b/c/g");
    CHECK(Url("./g").resolved(base).string() == "http://a/b/c/g");
    CHECK(Url("g/").resolved(base).string() == "http://a/b/c/g/");
    CHECK(Url("/g").resolved(base).string() == "http://a/g");
    CHECK(Url("?y").resolved(base).string() == "http://a/b/c/d;p?y");
    CHECK(Url("g?y").resolved(base).string() == "http://a/b/c/g?y");
    CHECK(Url("#s").resolved(base).string() == "http://a/b/c/d;p?q#s");
    CHECK(Url("g#s").resolved(base).string() == "http://a/b/c/g#s");
    CHECK(Url("g?y#s").resolved(base).string() == "http://a/b/c/g?y#s");
    CHECK(Url(";x").resolved(base).string() == "http://a/b/c/d;x"); // See [1] below.
    CHECK(Url("g;x").resolved(base).string() == "http://a/b/c/g;x");
    CHECK(Url("g;x?y#s").resolved(base).string() == "http://a/b/c/g;x?y#s");
    CHECK(Url("").resolved(base).string() == "http://a/b/c/d;p?q");

}

TEST_CASE("Resolve a URL against a relative base URL", "[Url]") {

    Url base("a/b/c/d;p?q");

    CHECK(Url("g:h").resolved(base).string() == "g:h");
    CHECK(Url("g").resolved(base).string() == "a/b/c/g");
    CHECK(Url("./g").resolved(base).string() == "a/b/c/g");
    CHECK(Url("g/").resolved(base).string() == "a/b/c/g/");
    CHECK(Url("/g").resolved(base).string() == "/g");
    CHECK(Url("?y").resolved(base).string() == "a/b/c/d;p?y");
    CHECK(Url("g?y").resolved(base).string() == "a/b/c/g?y");
    CHECK(Url("#s").resolved(base).string() == "a/b/c/d;p?q#s");
    CHECK(Url("g#s").resolved(base).string() == "a/b/c/g#s");
    CHECK(Url("g?y#s").resolved(base).string() == "a/b/c/g?y#s");
    CHECK(Url(";x").resolved(base).string() == "a/b/c/d;x"); // See [1] below.
    CHECK(Url("g;x").resolved(base).string() == "a/b/c/g;x");
    CHECK(Url("g;x?y#s").resolved(base).string() == "a/b/c/g;x?y#s");
    CHECK(Url("").resolved(base).string() == "a/b/c/d;p?q");

}

TEST_CASE("Resolve a relative URL against an empty base URL", "[Url]") {

    Url base("");

    CHECK(Url("g:h").resolved(base).string() == "g:h");
    CHECK(Url("g").resolved(base).string() == "g");
    CHECK(Url("./g").resolved(base).string() == "g");
    CHECK(Url("g/").resolved(base).string() == "g/");
    CHECK(Url("/g").resolved(base).string() == "/g");
    CHECK(Url("?y").resolved(base).string() == "?y");
    CHECK(Url("g?y").resolved(base).string() == "g?y");
    CHECK(Url("#s").resolved(base).string() == "#s");
    CHECK(Url("g#s").resolved(base).string() == "g#s");
    CHECK(Url("g?y#s").resolved(base).string() == "g?y#s");
    CHECK(Url(";x").resolved(base).string() == ";x");
    CHECK(Url("g;x").resolved(base).string() == "g;x");
    CHECK(Url("g;x?y#s").resolved(base).string() == "g;x?y#s");
    CHECK(Url("").resolved(base).string() == "");

}

TEST_CASE("Resolve an abnormal relative URL against an absolute base URL", "[Url]") {

    // https://tools.ietf.org/html/rfc3986#section-5.4.2

    Url base("http://a/b/c/d;p?q");

    CHECK(Url("../../../g").resolved(base).string() == "http://a/g");
    CHECK(Url("../../../../g").resolved(base).string() == "http://a/g");
    CHECK(Url("/./g").resolved(base).string() == "http://a/g");
    CHECK(Url("/../g").resolved(base).string() == "http://a/g");
    CHECK(Url("g.").resolved(base).string() == "http://a/b/c/g.");
    CHECK(Url(".g").resolved(base).string() == "http://a/b/c/.g");
    CHECK(Url("g..").resolved(base).string() == "http://a/b/c/g..");
    CHECK(Url("..g").resolved(base).string() == "http://a/b/c/..g");
    CHECK(Url("./../g").resolved(base).string() == "http://a/b/g");
    CHECK(Url("./g/.").resolved(base).string() == "http://a/b/c/g/");
    CHECK(Url("g/./h").resolved(base).string() == "http://a/b/c/g/h");
    CHECK(Url("g/../h").resolved(base).string() == "http://a/b/c/h");
    CHECK(Url("g;x=1/./y").resolved(base).string() == "http://a/b/c/g;x=1/./y"); // See [1] below.
    CHECK(Url("g;x=1/../y").resolved(base).string() == "http://a/b/c/g;x=1/../y"); // See [1] below.

}

// [1]:
// Some of the examples for path resolution given in RFC 3986 don't produce the same result
// in our implementation because the interpretation of the parameters string in RFC 3986 is
// in conflict with RFC 1808, which many URL utilities adhere to (e.g. NSURL). In RFC 1808
// the 'path' component stops at a ';', but in RFC 3986 it goes up to a '?'.
