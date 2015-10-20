#include "filters.h"
#include "scene/styleContext.h"
#include "data/tileData.h"

namespace Tangram {

bool Filter::eval(const Feature& feat, const StyleContext& ctx) const {

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
        return f.exists == feat.props.contains(f.key);
    }
    case FilterType::equality: {
        auto& f = data.get<Equality>();

        if (f.global == FilterGlobal::undefined) {
            auto& value = feat.props.get(f.key);
            for (const auto& v : f.values) {
                if (v == value) { return true; }
            }
        } else {
            auto& global = ctx.getGlobal(f.key);
            if (!global.is<none_type>()) {
                for (const auto& v : f.values) {
                    if (v == global) { return true; }
                }
                return false;
            }
        }

        return false;
    }
    case FilterType::range: {
        auto& f = data.get<Range>();

        if (f.global == FilterGlobal::undefined) {
            auto& value = feat.props.get(f.key);
            if (value.is<float>()) {
                auto num =  value.get<float>();
                return num >= f.min && num < f.max;
            }
            if (value.is<int64_t>()) {
                auto num =  value.get<int64_t>();
                return num >= f.min && num < f.max;
            }
        } else {
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
            }
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
    // Cannot be reached
    assert(false);
    return false;
}

}
