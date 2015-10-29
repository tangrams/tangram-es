#pragma once

#include "util/variant.h"

#include <vector>

namespace Tangram {

class StyleContext;
struct Feature;

enum class Operators : uint8_t {
    any = 0,
    all,
    none
};

enum class FilterType : uint8_t {
    any = 0,
    all,
    none,
    existence,
    equality,
    range,
    function,
    undefined
};

enum class FilterGlobal : uint8_t {
    undefined,
    zoom,
    geometry,
};

struct Filter {
    struct Operator {
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
    using Data = variant<none_type, Operator, Equality, Range, Existence, Function>;
    Data data;
    FilterType type;

    Filter() : data(none_type{}), type(FilterType::undefined) {}
    Filter(FilterType _type, Data _data) : data(std::move(_data)), type(_type) {}

    // Create an 'any', 'all', or 'none' filter
    inline static Filter MatchAny(const std::vector<Filter>& filters) {
        return { FilterType::any,  Operator{ filters }};
    }
    inline static Filter MatchAll(const std::vector<Filter>& filters) {
        return { FilterType::all,  Operator{ filters }};
    }
    inline static Filter MatchNone(const std::vector<Filter>& filters) {
        return { FilterType::none, Operator{ filters }};
    }
    // Create an 'equality' filter
    inline static Filter MatchEquality(const std::string& k, const std::vector<Value>& vals) {
        return { FilterType::equality, Equality{ k, vals, globalType(k) }};
    }
    // Create a 'range' filter
    inline static Filter MatchRange(const std::string& k, float min, float max) {
        return { FilterType::range, Range{ k, min, max, globalType(k) }};
    }
    // Create an 'existence' filter
    inline static Filter MatchExistence(const std::string& k, bool ex) {
        return { FilterType::existence, Existence{ k, ex }};
    }
    // Create an 'function' filter with reference to Scene function id
    inline static Filter MatchFunction(uint32_t id) {
        return { FilterType::function, Function{ id }};
    }

    bool eval(const Feature& feat, const StyleContext& ctx) const;

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
