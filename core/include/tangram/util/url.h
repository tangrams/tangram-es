#pragma once

#include <string>

namespace Tangram {

// This class is based on the URL concept specified by IETF RFC 1808
// (https://tools.ietf.org/html/rfc1808) and the URI concept specified by
// IETF RFC 3986 (https://tools.ietf.org/html/rfc3986) In particular this
// class is intended to handle URLs using the 'http' and 'file' schemes,
// with special case handling of some data URIs.
//
// URLs are decomposed as:
//
// foo://user:pword@host.com:80/over/there;type=a?name=ferret#nose
// \_/   \____________________/\_________/ \____/ \_________/ \__/
//  |              |               |         |         |       |
// scheme      netLocation        path   parameters  query  fragment
//
// Data URIs are decomposed as:
//
// data:image/png;base64,iVBORw0KGgoAAAANSUhE... (abbreviated)
// \__/ \_______/ \____/ \__________________ _ _
//  |       |       |         |
// scheme mediaType isBase64 data
//
// Ideas were borrowed from:
// https://github.com/cpp-netlib/uri/blob/master/include/network/uri/uri.hpp
// https://github.com/opensource-apple/CF/blob/master/CFURL.h
//

class Url {

public:

    // Create an empty URL.
    Url();

    // Create an absolute or relative URL from a string.
    Url(const std::string& source);
    Url(std::string&& source);
    Url(const char* source);

    // Create a URL by copy.
    Url(const Url& other);

    // Create a URL by move.
    Url(Url&& other);

    // Replace the contents of this URL.
    Url& operator=(const Url& other);
    Url& operator=(Url&& other);

    // Compare this URL and another using their string representations.
    bool operator==(const Url& other) const;

    // Query the state of this URL.
    bool isEmpty() const;
    bool isAbsolute() const;
    bool isStandardized() const;
    bool hasHttpScheme() const;
    bool hasFileScheme() const;
    bool hasDataScheme() const;
    bool hasBase64Data() const;

    // Query the presence of URL components.
    bool hasScheme() const;
    bool hasNetLocation() const;
    bool hasPath() const;
    bool hasParameters() const;
    bool hasQuery() const;
    bool hasFragment() const;

    // Query the presence of data URI components.
    bool hasMediaType() const;
    bool hasData() const;

    // Get copies of URL components.
    std::string scheme() const;
    std::string netLocation() const;
    std::string path() const;
    std::string parameters() const;
    std::string query() const;
    std::string fragment() const;

    // Get copies of data URI components.
    std::string mediaType() const;
    std::string data() const;

    // Get the entire URL as a string.
    const std::string& string() const;

    // Get an equivalent URL with dot segments removed from the path. If this is
    // a data URI then the same URI is returned.
    Url standardized() const;

    // Get an absolute URL by applying this URL relative to the given base.
    // e.g. "example.com/a/b/c.txt" == ("b/c.txt").resolved("example.com/a/")
    Url resolved(const Url& base) const;

    // Get an absolute URL by applying a relative URL to a base URL.
    // e.g. "example.com/a/b/c.txt" == resolve("example.com/a/", "b/c.txt")
    static Url resolve(const Url& base, const Url& relative);

    // Remove any '.' or '..' segments from a string containing a heirarchical path
    // and return a modified copy of the string.
    static std::string removeDotSegmentsFromString(std::string path);

private:

    // buffer contains the actual text of the URL.
    std::string buffer;

    // parts describes URL components by their location within the buffer.
    struct Parts {
        struct Range {
            size_t start = 0, count = 0;
        } scheme, location, path, parameters, query, fragment, media, data;
    } parts;

    // flags contains Boolean information about the URL state.
    int flags = 0;

    // flags uses these bitmasks.
    enum {
        IS_ABSOLUTE =     1 << 0,
        IS_STANDARDIZED = 1 << 1,
        HAS_HTTP_SCHEME = 1 << 2,
        HAS_FILE_SCHEME = 1 << 3,
        HAS_DATA_SCHEME = 1 << 4,
        HAS_BASE64_DATA = 1 << 5,
    };

    // parse the URL parts and flags from the source text.
    void parse();

    // Remove the last segment and its preceding '/' (if any) from a string range
    // containing a heirarchical path and return the index past the end of the new path.
    static size_t removeLastSegmentFromRange(std::string& string, size_t start, size_t count);

    // Remove any '.' or '..' segments from a string range containing a heirarchical
    // path and return the size of the new path.
    static size_t removeDotSegmentsFromRange(std::string& string, size_t start, size_t count);

}; // class Url

} // namespace Tangram

// A hash function allows Tangram::Url to be a key type for STL containers.
// To match the '==' operator, the hash function should only use the string value.
namespace std {
    template <>
    struct hash<Tangram::Url> {
        size_t operator()(const Tangram::Url& url) const {
            return std::hash<std::string>{}(url.string());
        }
    };
}
