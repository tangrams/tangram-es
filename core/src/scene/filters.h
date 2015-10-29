#pragma once

#include "util/variant.h"

#include <vector>

namespace Tangram {

class StyleContext;
struct Feature;

enum class FilterGlobal : uint8_t {
    undefined,
    zoom,
    geometry,
};

struct Filter {
    struct OperatorAll {
        std::vector<Filter> operands;
    };
    struct OperatorAny {
        std::vector<Filter> operands;
    };
    struct OperatorNone {
        std::vector<Filter> operands;
    };

    struct Equality {
        std::string key;
        std::vector<Value> values;
        FilterGlobal global;
    };
    struct Range {
        std::string key;
        float min;
        float max;
        FilterGlobal global;
    };
    struct Existence {
        std::string key;
        bool exists;
    };
    struct Function {
        uint32_t id;
    };
    using Data = variant<none_type,
                         OperatorAll,
                         OperatorNone,
                         OperatorAny,
                         Equality,
                         Range,
                         Existence,
                         Function>;
    Data data;

    Filter() : data(none_type{}) {}
    Filter(Data _data) : data(std::move(_data)) {}

    // Create an 'any', 'all', or 'none' filter
    inline static Filter MatchAny(const std::vector<Filter>& filters) {
        return { OperatorAny{ filters }};
    }
    inline static Filter MatchAll(const std::vector<Filter>& filters) {
        return { OperatorAll{ filters }};
    }
    inline static Filter MatchNone(const std::vector<Filter>& filters) {
        return { OperatorNone{ filters }};
    }
    // Create an 'equality' filter
    inline static Filter MatchEquality(const std::string& k, const std::vector<Value>& vals) {
        return { Equality{ k, vals, globalType(k) }};
    }
    // Create a 'range' filter
    inline static Filter MatchRange(const std::string& k, float min, float max) {
        return { Range{ k, min, max, globalType(k) }};
    }
    // Create an 'existence' filter
    inline static Filter MatchExistence(const std::string& k, bool ex) {
        return { Existence{ k, ex }};
    }
    // Create an 'function' filter with reference to Scene function id
    inline static Filter MatchFunction(uint32_t id) {
        return { Function{ id }};
    }

    bool eval(const Feature& feat, StyleContext& ctx) const;

private:

    static FilterGlobal globalType(const std::string& _key) {
        if (_key == "$geometry") {
            return FilterGlobal::geometry;
        } else if (_key == "$zoom") {
            return  FilterGlobal::zoom;
        }
        return  FilterGlobal::undefined;
    }
};
}
