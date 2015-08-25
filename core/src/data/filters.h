#pragma once

#include "tileData.h"
#include "scene/filterContext.h"

#include <unordered_map>
#include <vector>

namespace Tangram {

    using Context = FilterContext;

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
        function
    };

    struct Filter {
        struct Operator {
            std::vector<Filter> operands;
        };
        struct Equality {
            std::string key;
            std::vector<Value> values;
        };
        struct Range {
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
            std::string source;
        };

        FilterType type;
        variant<none_type, Operator, Equality, Range, Existence, Function> data;

        Filter() : data(none_type{}) {}

        // Create an 'any', 'all', or 'none' filter
        Filter(Operators op, const std::vector<Filter>& filters) :
            type(static_cast<FilterType>(op)), data(Operator{ filters }) {}

        // Create an 'equality' filter
        Filter(const std::string& k, const std::vector<Value>& vals) :
            type(FilterType::equality), data(Equality{ k, vals }) {}

        // Create a 'range' filter
        Filter(const std::string& k, float min, float max) :
            type(FilterType::range), data(Range{ k, min, max }) {}

        // Create an 'existence' filter
        Filter(const std::string& k, bool ex) :
            type(FilterType::existence), data(Existence{ k, ex }) {}

        // Create an 'function' filter
        Filter(uint32_t id, const std::string& source) :
            type(FilterType::function), data(Function{ id, source }) {}

        bool eval(const Feature& feat, const Context& ctx) const {

            switch (type) {

                case FilterType::any: {
                    for (const auto& filt : data.get<Operator>().operands) {
                        if (filt.eval(feat, ctx)) { return true; }
                    }
                    return false;
                }
                case FilterType::all: {
                    for (const auto& filt : data.get<Operator>().operands) {
                        if (!filt.eval(feat, ctx)) { return false; }
                    }
                    return true;
                }
                case FilterType::none: {
                    for (const auto& filt : data.get<Operator>().operands) {
                        if (filt.eval(feat, ctx)) { return false; }
                    }
                    return true;
                }
                case FilterType::existence: {
                    auto& f = data.get<Existence>();
                    bool found = ctx.globals.find(f.key) != ctx.globals.end() || feat.props.contains(f.key);

                    return f.exists == found;
                }
                case FilterType::equality: {
                    auto& f = data.get<Equality>();

                    auto ctxIt = ctx.globals.find(f.key);
                    if (ctxIt != ctx.globals.end()) {
                        for (const auto& v : f.values) {
                            if (v == ctxIt->second) { return true; }
                        }
                        return false;
                    }

                    auto& value = feat.props.get(f.key);
                    if (value.is<std::string>()) {
                        const auto& str = value.get<std::string>();
                        for (const auto& v : f.values) {
                            if (v == str) { return true; }
                        }
                    } else if (value.is<float>()) {
                        float num =  value.get<float>();
                        for (const auto& v : f.values) {
                            if (v == num) { return true; }
                        }
                    }

                    return false;
                }
                case FilterType::range: {
                    auto& f = data.get<Range>();

                    auto ctxIt = ctx.globals.find(f.key);
                    if (ctxIt != ctx.globals.end()) {
                        const auto& val = ctxIt->second;
                         // only check range for numbers
                        if (!val.is<float>()) { return false; }
                        return val.get<float>() >= f.min && val.get<float>() < f.max;
                    }
                    auto& value = feat.props.get(f.key);
                    if (value.is<float>()) {
                        float num =  value.get<float>();
                        return num >= f.min && num < f.max;
                    }

                    return false;
                }
                case FilterType::function: {
                    auto& f = data.get<Function>();
                    return ctx.evalFilter(f.id);
                }
                default:
                    return true;
            }
        }
    };
}
