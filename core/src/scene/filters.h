#pragma once

#include "util/variant.h"

#include <memory>
#include <vector>

namespace Tangram {

class StyleContext;
struct Feature;

struct Filter {
    static const std::string key_geom;
    static const std::string key_zoom;
    static const std::string key_other;
    static const std::vector<std::string> geometryStrings;

    enum class Key : uint8_t {
        other,
        zoom,
        geometry,
    };

    struct OperatorAll {
        std::vector<Filter> operands;
    };
    struct OperatorAny {
        std::vector<Filter> operands;
    };
    struct OperatorNone {
        std::vector<Filter> operands;
    };
    struct EqualitySet {
        std::string key;
        std::vector<Value> values;
    };
    struct Equality {
        std::string key;
        Value value;
    };
    struct EqualityKeySet {
        Key key;
        std::vector<Value> values;
    };
    struct EqualityKey {
        Key key;
        Value value;
    };
    struct Range {
        std::string key;
        float min;
        float max;
        bool hasPixelArea;
    };
    struct RangeKey {
        Key key;
        float min;
        float max;
        bool hasPixelArea;
    };
    struct Existence {
        std::string key;
        bool exists;
    };
    struct Function {
        uint32_t id;
    };
    using Data = variant<none_type,
                         Equality,
                         EqualityKey,
                         Range,
                         RangeKey,
                         Function,
                         Existence,
                         OperatorAll,
                         OperatorNone,
                         OperatorAny,
                         EqualitySet,
                         EqualityKeySet>;
    Data data;

    Filter() : data(none_type{}) {}
    Filter(Data _data) : data(std::move(_data)) {}

    bool eval(const Feature& feat, StyleContext& ctx) const;

    // Create an 'any', 'all', or 'none' filter
    inline static Filter MatchAny(std::vector<Filter> filters) {
        sort(filters);
        return { OperatorAny{ std::move(filters) }};
    }
    inline static Filter MatchAll(std::vector<Filter> filters) {
        sort(filters);
        return { OperatorAll{ std::move(filters) }};
    }
    inline static Filter MatchNone(std::vector<Filter> filters) {
        sort(filters);
        return { OperatorNone{ std::move(filters) }};
    }
    // Create an 'equality' filter
    inline static Filter MatchEquality(const std::string& k, const std::vector<Value>& vals) {
        auto key = keyType(k);
        if (key == Key::other) {
            if (vals.size() == 1) {
                return { Equality{ k, vals[0]}};
            } else {
                return { EqualitySet{ k, vals }};
            }
        } else {
            // FIXME check if the value is valid for the key!
            if (vals.size() == 1) {
                return { EqualityKey{ key, vals[0]}};
            } else {
                return { EqualityKeySet{ key, vals }};
            }
        }
    }
    // Create a 'range' filter
    inline static Filter MatchRange(const std::string& k, float min, float max, bool sqA) {
        auto key = keyType(k);
        if (key == Key::other) {
            return { Range{ k, min, max, sqA }};
        } else {
            return { RangeKey{ key, min, max, sqA }};
        }
    }
    // Create an 'existence' filter
    inline static Filter MatchExistence(const std::string& k, bool ex) {
        return { Existence{ k, ex }};
    }
    // Create an 'function' filter with reference to Scene function id
    inline static Filter MatchFunction(uint32_t id) {
        return { Function{ id }};
    }

    static Key keyType(const std::string& _key) {
        if (_key == key_geom) {
            return Key::geometry;
        } else if (_key == key_zoom) {
            return  Key::zoom;
        }
        return Key::other;
    }
    static const std::string& keyName(Key _key) {
        switch(_key) {
        case Key::geometry:
            return key_geom;
        case Key::zoom:
            return key_zoom;
        default:
            break;
        }
        return key_other;
    }

    /* Public for testing */
    static void sort(std::vector<Filter>& filters);
    void print(int _indent = 0) const;
    int filterCost() const;
    bool isOperator() const;
    const std::string& key() const;
    const std::vector<Filter>& operands() const;

    bool isValid() const { return !data.is<none_type>(); }
    operator bool() const { return isValid(); }
};
}
