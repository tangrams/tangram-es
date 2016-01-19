#include "filters.h"
#include "scene/styleContext.h"
#include "data/tileData.h"
#include "platform.h"

#include <cmath>

namespace Tangram {

void countTypes(const std::vector<Filter>& filters, int& global, int& function, int& property) {

}

void Filter::print(int _indent) const {

    switch (data.get_type_index()) {

    case Data::type<OperatorAny>::value: {
        logMsg("%*s any\n", _indent, "");
        for (const auto& filt : data.get<OperatorAny>().operands) {
            filt.print(_indent + 2);
        }
        break;
    }
    case Data::type<OperatorAll>::value: {
        logMsg("%*s all\n", _indent, "");
        for (const auto& filt : data.get<OperatorAll>().operands) {
            filt.print(_indent + 2);
        }
        break;
    }
    case Data::type<OperatorNone>::value: {
        logMsg("%*s none\n", _indent, "");
        for (const auto& filt : data.get<OperatorNone>().operands) {
            filt.print(_indent + 2);
        }
        break;
    }
    case Data::type<Existence>::value: {
        auto& f = data.get<Existence>();
        logMsg("%*s existence - key:%s\n", _indent, "", f.key.c_str());
        break;
    }
    case Data::type<Equality>::value: {
        auto& f = data.get<Equality>();
        if (f.values[0].is<std::string>()) {
            logMsg("%*s equality - global:%d key:%s val:%s\n", _indent, "",
                   f.global != FilterGlobal::undefined,
                   f.key.c_str(),
                   f.values[0].get<std::string>().c_str());
        }
        if (f.values[0].is<double>()) {
            logMsg("%*s equality - global:%d key:%s val:%f\n", _indent, "",
                   f.global != FilterGlobal::undefined,
                   f.key.c_str(),
                   f.values[0].get<double>());
        }
        break;
    }
    case Data::type<Range>::value: {
        auto& f = data.get<Range>();
        logMsg("%*s range - global:%d key:%s min:%f max:%f\n", _indent, "",
               f.global != FilterGlobal::undefined,
               f.key.c_str(), f.min, f.max);
        return;
    }
    case Data::type<Function>::value: {
        logMsg("%*s function\n", _indent, "");
        break;
    }
    default:
        break;
    }

}


int Filter::matchCost() const {
    // Add some extra penalty for set vs simple filters
    int sum = -100;

    switch (data.get_type_index()) {
    case Data::type<OperatorAny>::value:
        for (auto& f : operands()) { sum -= f.matchCost(); }
        return sum;

    case Data::type<OperatorAll>::value:
        for (auto& f : operands()) { sum -= f.matchCost(); }
        return sum;

    case Data::type<OperatorNone>::value:
        for (auto& f : operands()) { sum -= f.matchCost(); }
        return sum;

    case Data::type<Existence>::value:
        // Equality and Range are more specific for increasing
        // the chance to fail early check them before Existence
        return 20;

    case Data::type<Equality>::value:
        return data.get<Equality>().global == FilterGlobal::undefined ? 10 : 1;

    case Data::type<Filter::Range>::value:
        return data.get<Range>().global == FilterGlobal::undefined ? 10 : 1;

    case Data::type<Function>::value:
        // Most expensive filter should be checked last
        return 1000;
    }
    assert(false);
    return 0;
}

const std::string& Filter::key() const {
    static const std::string empty = "";

    switch (data.get_type_index()) {

    case Data::type<Existence>::value:
        return data.get<Existence>().key;

    case Data::type<Equality>::value:
        return data.get<Equality>().key;

    case Data::type<Filter::Range>::value:
        return data.get<Range>().key;

    default:
        break;
    }
    return empty;
}

const std::vector<Filter>& Filter::operands() const {
    static const std::vector<Filter> empty;

    switch (data.get_type_index()) {
    case Data::type<OperatorAny>::value:
        return data.get<OperatorAny>().operands;

    case Data::type<OperatorAll>::value:
        return data.get<OperatorAll>().operands;

    case Data::type<OperatorNone>::value:
        return data.get<OperatorNone>().operands;

    default:
        break;
    }
    return empty;
}

int compareSetFilter(const Filter& a, const Filter& b) {
    auto& oa = a.operands();
    auto& ob = b.operands();

    if (oa.size() != ob.size()) { return oa.size() < ob.size(); }

    if (oa[0].data.is<Filter::Range>() &&
        ob[0].data.is<Filter::Range>() &&
        oa[0].key() == ob[0].key()) {
        // take the one with more restrictive range
        auto ra = oa[0].data.get<Filter::Range>();
        auto rb = ob[0].data.get<Filter::Range>();

        if (ra.max == std::numeric_limits<double>::infinity() &&
            rb.max == std::numeric_limits<double>::infinity()) {

            return rb.min - ra.min;
        }
    }

    return 0;
}

std::vector<Filter> Filter::sort(const std::vector<Filter>& _filters) {
    std::vector<Filter> filters = _filters;
    std::sort(filters.begin(), filters.end(),
              [](auto& a, auto& b) {

                  // Sort simple filters by eval cost
                  int ma = a.matchCost();
                  int mb = b.matchCost();
                  if (ma > 0 && mb > 0) {
                      int diff = ma - mb;
                      if (diff != 0) {
                          return diff < 0;
                      }

                      // just for consistent ordering
                      // (and using > to prefer $zoom over $geom)
                      return a.key() > b.key();
                  }

                  // When one is a simple Filter and the other is a set
                  // or both are sets prefer the one with the cheaper
                  // filter(s).
                  if (ma != mb) {
                      // No abs(int) in our android libstdc..
                      //return std::abs(ma) < std::abs(mb);
                      return std::fabs(ma) < std::fabs(mb);
                  }

                  return compareSetFilter(a, b) < 0;
              });

    return filters;
}

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
                if (v == value) {
                    return true;
                } else if (value.is<double>() && v.is<double>()) {
                    auto& a = v.get<double>();
                    auto& b = value.get<double>();
                    if (std::fabs(a - b) <= std::numeric_limits<double>::epsilon()) { return true; }
                }
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
            if (value.is<double>()) {
                double num =  value.get<double>();
                return num >= f.min && num < f.max;
            }
        } else {
            auto& global = ctx.getGlobal(f.global);
            if (!global.is<none_type>()) {
                // only check range for numbers
                if (global.is<double>()) {
                    double num = global.get<double>();
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
    //assert(false);
    return false;
}

}
