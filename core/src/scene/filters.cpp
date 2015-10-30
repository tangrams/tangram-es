#include "filters.h"
#include "scene/styleContext.h"
#include "data/tileData.h"

namespace Tangram {

bool Filter::eval(const Feature& feat, StyleContext& ctx) const {

    switch (data.get_type_index()) {

    case Data::type<OperatorAny>::value: {
        for (const auto& filt : data.get<OperatorAny>().operands) {
            if (filt.eval(feat, ctx)) { return true; }
        }
        return false;
    }
    case Data::type<OperatorAll>::value: {
        for (const auto& filt : data.get<OperatorAll>().operands) {
            if (!filt.eval(feat, ctx)) { return false; }
        }
        return true;
    }
    case Data::type<OperatorNone>::value: {
        for (const auto& filt : data.get<OperatorNone>().operands) {
            if (filt.eval(feat, ctx)) { return false; }
        }
        return true;
    }
    case Data::type<Existence>::value: {
        auto& f = data.get<Existence>();
        return f.exists == feat.props.contains(f.key);
    }
    case Data::type<Equality>::value: {
        auto& f = data.get<Equality>();

        if (f.global == FilterGlobal::undefined) {
            auto& value = feat.props.get(f.key);
            for (const auto& v : f.values) {
                if (v == value) { return true; }
            }
        } else {
            auto& global = ctx.getGlobal(f.global);
            if (!global.is<none_type>()) {
                for (const auto& v : f.values) {
                    if (v == global) { return true; }
                }
                return false;
            }
        }

        return false;
    }
    case Data::type<Range>::value: {
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
            auto& global = ctx.getGlobal(f.global);
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
    case Data::type<Function>::value: {
        auto& f = data.get<Function>();
        return ctx.evalFilter(f.id);
    }
    default:
        return true;
    }

    // Cannot be reached
    assert(false);
    return false;
}

}
