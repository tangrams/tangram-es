#pragma once

#include "util/variant.h"

#include <vector>

namespace Tangram {

class StyleContext;
struct Feature;

enum class Operators : int {
    any = 0,
    all,
    none
};

enum class FilterType : int {
    any = 0,
    all,
    none,
    existence,
    equality,
    range,
    function,
    undefined
};

enum class FilterGlobal : int {
    undefined,
    zoom,
    geometry,
};

struct Filter {
    struct Operator {
        std::vector<Filter> operands;
    };
    struct Equality {
        FilterGlobal global;
        std::string key;
        std::vector<Value> values;
    };
    struct Range {
        FilterGlobal global;
        std::string key;
        float min;
        float max;
    };
    struct Existence {
        std::string key;
        bool exists;
    };
    struct Function {
        uint32_t id;
    };

    FilterType type;
    using Data = variant<none_type, Operator, Equality, Range, Existence, Function>;
    Data data;

    Filter() : type(FilterType::undefined), data(none_type{}) {}
    Filter(FilterType _type, Data _data) : type(_type), data(std::move(_data)) {}

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
        return { FilterType::equality, Equality{ globalType(k), k, vals }};
    }
    // Create a 'range' filter
    inline static Filter MatchRange(const std::string& k, float min, float max) {
        return { FilterType::range, Range{ globalType(k), k, min, max }};
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
