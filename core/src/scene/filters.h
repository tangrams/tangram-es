#pragma once

#include "util/variant.h"

#include <memory>
#include <vector>

namespace YAML {
class Node;
}

namespace Tangram {

class StyleContext;
struct SceneFunctions;
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

    enum class Geometry : uint8_t {
        unknown,
        point,
        line,
        polygon,
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
    struct EqualityKey {
        Key key;
        int value;
    };
    struct EqualityString {
        std::string key;
        std::string value;
    };
    struct EqualityNumber {
        std::string key;
        double value;
    };
    // struct EqualityKeySet {
    //     Key key;
    //     std::vector<int> values;
    // };
    struct EqualityStringSet {
        std::string key;
        std::vector<std::string> values;
    };
    struct EqualityNumberSet {
        std::string key;
        std::vector<double> values;
    };
    struct Range {
        std::string key;
        float min;
        float max;
    };
    struct RangeArea {
        std::string key;
        float min;
        float max;
    };
    struct RangeKeyZoom {
        Key key;
        int min;
        int max;
    };
    struct Existence {
        std::string key;
        bool exists;
    };
    struct Function {
        uint32_t id;
    };


    bool eval(const Feature& feat, StyleContext& ctx) const;

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

    using Data =
        variant<none_type,
                EqualityKey,
                EqualityString,
                EqualityNumber,
                RangeKeyZoom,
                Range,
                RangeArea,
                // EqualityKeySet,
                EqualityStringSet,
                EqualityNumberSet,
                Existence,
                Function,
                OperatorAll,
                OperatorAny,
                OperatorNone
                >;
    Data data;
    explicit Filter(Data&& _data) : data(std::move(_data)) {}
    static int compareSetFilter(const Filter& a, const Filter& b);
    struct matcher;

public:
    Filter() : data(none_type{}) {}

    // Create an 'equality' filter
    static Filter getEqualityStringFilter(const std::string& k, const std::vector<std::string>& vals);
    static Filter getEqualityNumberFilter(const std::string& k, const std::vector<double>& vals);

    // Create a 'range' filter
    static Filter getRangeFilter(const std::string& k, float min, float max, bool sqArea);

    // Create an 'existence' filter
    static Filter getExistenceFilter(const std::string& k, bool ex) {
        return Filter{ Existence{ k, ex }};
    }
    // Create an 'function' filter with reference to Scene function id
    static Filter getFunctionFilter(uint32_t id) {
        return Filter{ Function{ id }};
    }

    static Filter generateFilter(const YAML::Node& _filter, SceneFunctions& _fns);
    static Filter generatePredicate(const YAML::Node& _value, std::string _key);
    static Filter generateAnyFilter(const YAML::Node& _filter, SceneFunctions& _fns);
    static Filter generateAllFilter(const YAML::Node& _filter, SceneFunctions& _fns);
    static Filter generateNoneFilter(const YAML::Node& _filter, SceneFunctions& _fns);
    static bool getFilterRangeValue(const YAML::Node& node, double& val, bool& hasPixelArea);

};
}
