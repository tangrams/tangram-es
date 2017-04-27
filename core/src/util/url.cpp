#include "util/url.h"

#include <cstdlib>
#include <cassert>

namespace Tangram {

Url::Url() {}

Url::Url(const std::string& source) :
    buffer(source) {
    parse();
}

Url::Url(std::string&& source) :
    buffer(source) {
    parse();
}

Url::Url(const char* source) :
    buffer(source) {
    parse();
}

Url::Url(const Url& other) :
    buffer(other.buffer),
    parts(other.parts),
    flags(other.flags) {
}

Url::Url(Url&& other) :
    buffer(std::move(other.buffer)),
    parts(other.parts),
    flags(other.flags) {
}

Url& Url::operator=(const Url& other) {
    buffer = other.buffer;
    parts = other.parts;
    flags = other.flags;
    return *this;
}

Url& Url::operator=(Url&& other) {
    buffer = std::move(other.buffer);
    parts = other.parts;
    flags = other.flags;
    return *this;
}

bool Url::operator==(const Url& rhs) const {
    return buffer == rhs.buffer;
}

bool Url::isEmpty() const {
    return buffer.empty();
}

bool Url::isAbsolute() const {
    return (flags & IS_ABSOLUTE);
}

bool Url::isStandardized() const {
    return (flags & IS_STANDARDIZED);
}

bool Url::hasHttpScheme() const {
    return (flags & HAS_HTTP_SCHEME);
}

bool Url::hasFileScheme() const {
    return (flags & HAS_FILE_SCHEME);
}

bool Url::hasDataScheme() const {
    return (flags & HAS_DATA_SCHEME);
}

bool Url::hasBase64Data() const {
    return (flags & HAS_BASE64_DATA);
}

bool Url::hasScheme() const {
    return parts.scheme.count != 0;
}

bool Url::hasNetLocation() const {
    return parts.location.count != 0;
}

bool Url::hasPath() const {
    return parts.path.count != 0;
}

bool Url::hasParameters() const {
    return parts.parameters.count != 0;
}

bool Url::hasQuery() const {
    return parts.query.count != 0;
}

bool Url::hasFragment() const {
    return parts.fragment.count != 0;
}

bool Url::hasMediaType() const {
    return parts.media.count != 0;
}

bool Url::hasData() const {
    return parts.data.count != 0;
}

std::string Url::scheme() const {
    return std::string(buffer, parts.scheme.start, parts.scheme.count);
}

std::string Url::netLocation() const {
    return std::string(buffer, parts.location.start, parts.location.count);
}

std::string Url::path() const {
    return std::string(buffer, parts.path.start, parts.path.count);
}

std::string Url::parameters() const {
    return std::string(buffer, parts.parameters.start, parts.parameters.count);
}

std::string Url::query() const {
    return std::string(buffer, parts.query.start, parts.query.count);
}

std::string Url::fragment() const {
    return std::string(buffer, parts.fragment.start, parts.fragment.count);
}

std::string Url::mediaType() const {
    return std::string(buffer, parts.media.start, parts.media.count);
}

std::string Url::data() const {
    return std::string(buffer, parts.data.start, parts.data.count);
}

const std::string& Url::string() const {
    return buffer;
}

Url Url::standardized() const {

    // If this URL is already standardized, return a copy.
    if (isStandardized()) {
        return *this;
    }

    // If this is a data URI, return a copy.
    if (hasDataScheme()) {
        return *this;
    }

    // Create the target URL by copying this URL.
    Url t(*this);

    // Remove any dot segments from the path.
    size_t count = removeDotSegmentsFromRange(t.buffer, t.parts.path.start, t.parts.path.count);

    if (count != t.parts.path.count) {

        // The new path should always be the same size or shorter.
        assert(count < t.parts.path.count);
        size_t offset = t.parts.path.count - count;

        // Adjust the size of the 'path' part.
        t.parts.path.count = count;

        // Remove any extra parts of the old path from the string.
        t.buffer.erase(t.parts.path.start + t.parts.path.count, offset);

        // Adjust the locations of the URL parts after 'path'.
        t.parts.parameters.start -= offset;
        t.parts.query.start -= offset;
        t.parts.fragment.start -= offset;
    }

    // Set the standardized flag.
    t.flags |= IS_STANDARDIZED;

    return t;
}

Url Url::resolved(const Url& base) const {
    return resolve(base, *this);
}

Url Url::resolve(const Url& b, const Url& r) {
    // https://tools.ietf.org/html/rfc1808#section-4
    // https://tools.ietf.org/html/rfc3986#section-5.2

    // If the base URL is empty or the relative URL is already absolute, return the relative URL.
    // This includes the case where the relative URL is a data URI.
    if (r.isAbsolute() || b.isEmpty()) {
        return r.standardized();
    }

    // If the relative URL is empty, return the base URL.
    if (r.isEmpty()) {
        return b;
    }

    // Start composing the new target URL.
    Url t;

    // Take the scheme from the base URL.
    if (b.hasScheme()) {
        t.parts.scheme = b.parts.scheme;
        t.buffer.append(b.buffer, b.parts.scheme.start, b.parts.scheme.count);
        t.buffer.append("://");
    }

    // Resolve the netLocation.
    t.parts.location.start = t.buffer.size();
    if (r.hasNetLocation()) {
        // If the relative URL has a netLocation, use it for the new URL.
        t.buffer.append(r.buffer, r.parts.location.start, r.parts.location.count);
    } else {
        // Use the netLocation of the base in the new URL.
        t.buffer.append(b.buffer, b.parts.location.start, b.parts.location.count);
    }
    t.parts.location.count = t.buffer.size() - t.parts.location.start;

    // Resolve the path.
    t.parts.path.start = t.buffer.size();
    if (r.hasNetLocation()) {
        // If the relative URL has a netLocation, use its path as well.
        t.buffer.append(r.buffer, r.parts.path.start, r.parts.path.count);
    } else if (r.hasPath()) {
        // If the relative URL's path starts with '/', it is absolute.
        if (r.buffer[r.parts.path.start] == '/') {
            t.buffer.append(r.buffer, r.parts.path.start, r.parts.path.count);
        } else {
            // Merge the relative path with the base path.
            if (b.hasNetLocation() && !b.hasPath()) {
                t.buffer += '/';
                t.buffer.append(r.buffer, r.parts.path.start, r.parts.path.count);
            } else {
                // Add the base URL's path.
                t.buffer.append(b.buffer, b.parts.path.start, b.parts.path.count);
                // Remove the last segment of the base URL's path, up to the last '/'.
                while (!t.buffer.empty() && t.buffer.back() != '/') { t.buffer.pop_back(); }
                // Add the relative URL's path.
                t.buffer.append(r.buffer, r.parts.path.start, r.parts.path.count);
            }
        }
    } else {
        // If the relative URL has no netLocation or path, use the base URL's path.
        t.buffer.append(b.buffer, b.parts.path.start, b.parts.path.count);
    }
    t.parts.path.count = t.buffer.size() - t.parts.path.start;

    // Remove dot segments from the resolved path.
    t.parts.path.count = removeDotSegmentsFromRange(t.buffer, t.parts.path.start, t.parts.path.count);
    t.buffer.resize(t.parts.path.start + t.parts.path.count);

    // Resolve the parameters.
    t.parts.parameters.start = t.buffer.size();
    if (r.hasParameters()) {
        // If the relative URL has parameters, use it in the new URL.
        t.buffer += ';';
        t.parts.parameters.start += 1;
        t.parts.parameters.count = r.parts.parameters.count;
        t.buffer.append(r.buffer, r.parts.parameters.start, r.parts.parameters.count);
    } else if (b.hasParameters() && !(r.hasNetLocation() || r.hasPath())) {
        // Use the base URL parameters.
        t.buffer += ';';
        t.parts.parameters.start += 1;
        t.parts.parameters.count = b.parts.parameters.count;
        t.buffer.append(b.buffer, b.parts.parameters.start, b.parts.parameters.count);
    }
    t.parts.parameters.count = t.buffer.size() - t.parts.parameters.start;

    // Resolve the query.
    t.parts.query.start = t.buffer.size();
    if (r.hasQuery()) {
        // If this URL has a query, use it in the new URL.
        t.buffer += '?';
        t.parts.query.start += 1;
        t.buffer.append(r.buffer, r.parts.query.start, r.parts.query.count);
    } else if (b.hasQuery() && !(r.hasNetLocation() || r.hasPath() || r.hasParameters())) {
        // Use the base URL query.
        t.buffer += '?';
        t.parts.query.start += 1;
        t.buffer.append(b.buffer, b.parts.query.start, b.parts.query.count);
    }
    t.parts.query.count = t.buffer.size() - t.parts.query.start;

    // Use the fragment from the relative URL.
    t.parts.fragment.start = t.buffer.size();
    if (r.hasFragment()) {
        t.buffer += '#';
        t.parts.fragment.start += 1;
        t.buffer.append(r.buffer, r.parts.fragment.start, r.parts.fragment.count);
    }
    t.parts.fragment.count = t.buffer.size() - t.parts.fragment.start;

    // Flags can be copied from the base URL.
    t.flags = b.flags;

    // The path had dot segments removed, so we can set the standardized flag.
    t.flags |= IS_STANDARDIZED;

    return t;
}

std::string Url::removeDotSegmentsFromString(std::string path) {
    auto count = removeDotSegmentsFromRange(path, 0, path.size());
    path.resize(count);
    return path;
}

void Url::parse() {

    // The parsing process roughly follows https://tools.ietf.org/html/rfc1808#section-2.4

    size_t start = 0;
    size_t end = buffer.size();

    // Parse the fragment.
    {
        // If there's a '#' in the string, the substring after it to the end is the fragment.
        auto pound = std::min(buffer.find('#', start), end);
        parts.fragment.start = std::min(pound + 1, end);
        parts.fragment.count = end - parts.fragment.start;

        // Remove the '#' and fragment from parsing.
        end = pound;
    }

    // Parse the scheme.
    {
        size_t i = start;
        auto c = buffer[i];

        // A scheme is permitted to contain only alphanumeric characters, '+', '.', and '-'.
        while (i < end && (isalnum(c) || c == '+' || c == '.' || c == '-')) {
            c = buffer[++i];
        }

        // If a scheme is present, it must be followed by a ':'.
        if (c == ':') {
            parts.scheme.start = start;
            parts.scheme.count = i - start;

            // Remove the scheme and ':' from parsing.
            start = i + 1;

            // Set the absolute URL flag.
            flags |= IS_ABSOLUTE;
        }
    }

    // If scheme is 'data', parse as data URI.
    if (buffer.compare(parts.scheme.start, parts.scheme.count, "data") == 0) {

        // Set the data scheme flag.
        flags |= HAS_DATA_SCHEME;

        // A data scheme will be followed by a media type, then either a comma or a base 64 indicator string.
        auto base64Indicator = std::min(buffer.find(";base64", start), end);
        auto comma = std::min(buffer.find(',', start), end);

        // If the base 64 indicator string is found before the comma, set the matching flag.
        if (base64Indicator < comma) {
            flags |= HAS_BASE64_DATA;
        }

        // The media type goes from the colon after the scheme up to either the comma or the base 64 string.
        parts.media.start = start;
        parts.media.count = std::min(base64Indicator, comma) - start;

        // The data section is separated by a comma and goes to the end of the URI.
        start = std::min(comma + 1, end);
        parts.data.start = start;
        parts.data.count = end - start;

        // We're done!
        return;
    }

    // Check whether the scheme is 'http', 'https', or 'file' and set appropriate flags.
    if (buffer.compare(parts.scheme.start, parts.scheme.count, "http") == 0 ||
        buffer.compare(parts.scheme.start, parts.scheme.count, "https") == 0) {
        flags |= HAS_HTTP_SCHEME;
    } else if (buffer.compare(parts.scheme.start, parts.scheme.count, "file") == 0) {
        flags |= HAS_FILE_SCHEME;
    }

    // If '//' is next in the string, then the substring up to the following '/' is the network location.
    if (buffer.compare(start, 2, "//") == 0) {
        start += 2;
        auto slash = std::min(buffer.find('/', start), end);
        parts.location.start = start;
        parts.location.count = slash - start;

        // Remove the network location from parsing.
        start = slash;
    }

    // Parse the query.
    {
        // If there's a '?' in the remaining string, the substring after it to the end is the query string.
        auto qmark = std::min(buffer.find('?', start), end);
        parts.query.start = std::min(qmark + 1, end);
        parts.query.count = end - parts.query.start;

        // Remove the '?' and query from parsing.
        end = qmark;
    }

    // Parse the parameters.
    {
        // If there's a ';' in the remaining string, the substring after it to the end is the parameters string.
        auto semicolon = std::min(buffer.find(';', start), end);
        parts.parameters.start = std::min(semicolon + 1, end);
        parts.parameters.count = end - parts.parameters.start;

        // Remove the ';' and parameters from parsing.
        end = semicolon;
    }

    // Parse the path. After the preceding steps, the remaining string is the URL path.
    parts.path.start = start;
    parts.path.count = end - start;

}

size_t Url::removeLastSegmentFromRange(std::string& string, size_t start, size_t end) {
    if (start >= end) { return start; }
    size_t pos = end - 1;
    while (pos > start && string[pos] != '/') { pos--; }
    return pos;
}

size_t Url::removeDotSegmentsFromRange(std::string& str, size_t start, size_t count) {

    // Implements https://tools.ietf.org/html/rfc3986#section-5.2.4
    // with in-place manipulation instead of making a new buffer.

    size_t end = start + count;
    size_t pos = start; // 'input' position.
    size_t out = pos; // 'output' position.

    while (pos < end) {
        if (pos + 2 < end &&
                   str[pos] == '.' &&
                   str[pos + 1] == '.' &&
                   str[pos + 2] == '/') {
            pos += 3;
        } else if (pos + 1 < end &&
                   str[pos] == '.' &&
                   str[pos + 1] == '/') {
            pos += 2;
        } else if (pos + 2 < end &&
                   str[pos] == '/' &&
                   str[pos + 1] == '.' &&
                   str[pos + 2] == '/') {
            pos += 2;
        } else if (pos + 2 == end &&
                   str[pos] == '/' &&
                   str[pos + 1] == '.') {
            pos += 1;
            str[pos] = '/';
        } else if (pos + 3 < end &&
                   str[pos] == '/' &&
                   str[pos + 1] == '.' &&
                   str[pos + 2] == '.' &&
                   str[pos + 3] == '/') {
            pos += 3;
            out = removeLastSegmentFromRange(str, start, out);
        } else if (pos + 3 == end &&
                   str[pos] == '/' &&
                   str[pos + 1] == '.' &&
                   str[pos + 2] == '.') {
            pos += 3;
            out = removeLastSegmentFromRange(str, start, out);
        } else if (pos + 1 == end &&
                   str[pos] == '.') {
            pos += 1;
        } else if (pos + 2 == end &&
                   str[pos] == '.' &&
                   str[pos + 1] == '.') {
            pos += 2;
        } else {
            do {
                str[out++] = str[pos++];
            } while (pos < end && str[pos] != '/');
        }
    }

    return out - start;
}

} // namespace Tangram
