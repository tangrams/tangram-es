#pragma once

#include "tileData.h"

#include <unordered_map>
#include <vector>

namespace Tangram {

    using Context = std::unordered_map<std::string, Value>;

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
        range
    };

    struct Filter {

        std::vector<Filter> operands;
        std::vector<Value> values;
        std::string key;
        bool exists;

        FilterType type;

        Filter() : type(FilterType::none) {}

        // Create an 'any', 'all', or 'none' filter
        Filter(Operators op, const std::vector<Filter>& filters) : operands(filters), type(static_cast<FilterType>(op)) {}

        // Create an 'equality' filter
        Filter(const std::string& k, const std::vector<Value>& vals) : values(vals), key(k), type(FilterType::equality) {}

        // Create a 'range' filter
        Filter(const std::string& k, float min, float max) : values({ Value(min), Value(max) }), key(k), type(FilterType::range) {}

        // Create an 'existence' filter
        Filter(const std::string& k, bool ex) : key(k), exists(ex), type(FilterType::existence) {}

        bool eval(const Feature& feat, const Context& ctx) const {

            switch (type) {

                case FilterType::any: {
                    for (const auto& filt : operands) {
                        if (filt.eval(feat, ctx)) { return true; }
                    }
                    return false;
                }
                case FilterType::all: {
                    for (const auto& filt : operands) {
                        if (!filt.eval(feat, ctx)) { return false; }
                    }
                    return true;
                }
                case FilterType::none: {
                    for (const auto& filt : operands) {
                        if (filt.eval(feat, ctx)) { return false; }
                    }
                    return true;
                }
                case FilterType::existence: {

                    bool found = ctx.find(key) != ctx.end() || feat.props.contains(key);

                    return exists == found;
                }
                case FilterType::equality: {

                    auto ctxIt = ctx.find(key);
                    if (ctxIt != ctx.end()) {
                        for (const auto& v : values) {
                            if (v == ctxIt->second) { return true; }
                        }
                        return false;
                    }

                    auto& value = feat.props.get(key);
                    if (value.is<std::string>()) {
                        const auto& str = value.get<std::string>();
                        for (const auto& v : values) {
                            if (v == str) { return true; }
                        }
                    } else if (value.is<float>()) {
                        float num =  value.get<float>();
                        for (const auto& v : values) {
                            if (v == num) { return true; }
                        }
                    }

                    return false;
                }
                case FilterType::range: {

                    float min = values[0].get<float>();
                    float max = values[1].get<float>();
                    auto ctxIt = ctx.find(key);
                    if (ctxIt != ctx.end()) {
                        const auto& val = ctxIt->second;
                        if (!val.is<float>()) { return false; } // only check range for numbers
                        return val.get<float>() >= min && val.get<float>() < max;
                    }
                    auto& value = feat.props.get(key);
                    if (value.is<float>()) {
                        float num =  value.get<float>();
                        return num >= min && num < max;
                    }

                    return false;
                }
                default:
                    return true;
            }
        }

    };
}
