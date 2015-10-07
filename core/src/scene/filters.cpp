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
            if (!global.is<float>()) { return false; }
            return global.get<float>() >= f.min && global.get<float>() < f.max;
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
    case FilterType::undefined:
        return true;
    }
    // Cannot be reached
    assert(false);
    return false;
}

}
