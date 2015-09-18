#pragma once

#include "tileData.h"
#include "scene/styleContext.h"

#include <unordered_map>
#include <vector>

namespace Tangram {

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
        };

        FilterType type;
        variant<none_type, Operator, Equality, Range, Existence, Function> data;

        Filter() : type(FilterType::undefined), data(none_type{}) {}

        // Create an 'any', 'all', or 'none' filter
        Filter(Operators op, const std::vector<Filter>& filters) :
            type(static_cast<FilterType>(op)),
            data(Operator{ filters }) {}

        // Create an 'equality' filter
        Filter(const std::string& k, const std::vector<Value>& vals) :
            type(FilterType::equality),
            data(Equality{ k, vals }) {}

        // Create a 'range' filter
        Filter(const std::string& k, float min, float max) :
            type(FilterType::range),
            data(Range{ k, min, max }) {}

        // Create an 'existence' filter
        Filter(const std::string& k, bool ex) :
            type(FilterType::existence),
            data(Existence{ k, ex }) {}

        // Create an 'function' filter with reference to Scene function id
        Filter(uint32_t id) :
            type(FilterType::function),
            data(Function{ id }) {}

        bool eval(const Feature& feat, const StyleContext& ctx) const {

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
                    auto& global = ctx.getGlobal(f.key);

                    bool found = !global.is<none_type>() || feat.props.contains(f.key);

                    return f.exists == found;
                }
                case FilterType::equality: {
                    auto& f = data.get<Equality>();
                    auto& global = ctx.getGlobal(f.key);

                    if (!global.is<none_type>()) {
                        for (const auto& v : f.values) {
                            if (v == global) { return true; }
                        }
                        return false;
                    }

                    auto& value = feat.props.get(f.key);
                    for (const auto& v : f.values) {
                        if (v == value) { return true; }
                    }

                    return false;
                }
                case FilterType::range: {
                    auto& f = data.get<Range>();
                    auto& global = ctx.getGlobal(f.key);

                    if (!global.is<none_type>()) {
                         // only check range for numbers
                        if (global.is<int64_t>()) {
                            auto num = global.get<int64_t>();
                            return num >= f.min && num < f.max;
                        }
                        if (global.is<float>()) {
                            auto num = global.get<float>();
                            return num >= f.min && num < f.max;
                        }
                        return false;
                    }
                    auto& value = feat.props.get(f.key);

                    if (value.is<int64_t>()) {
                        auto num = value.get<int64_t>();
                        return num >= f.min && num < f.max;
                    }
                    if (value.is<float>()) {
                        auto num = value.get<float>();
                        return num >= f.min && num < f.max;
                    }
                    return false;
                }
                case FilterType::function: {
                    auto& f = data.get<Function>();
                    return ctx.evalFilter(f.id);
                }
                case FilterType::undefined:
                    return true;
            }
        }
    };
}
