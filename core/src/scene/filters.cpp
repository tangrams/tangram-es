#include "filters.h"
#include "scene/sceneLayer.h"
#include "scene/styleContext.h"
#include "data/tileData.h"
#include "platform.h"

#include <cmath>

namespace Tangram {

void Filter::print(int _indent) const {

    switch (data.which()) {

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
    case Data::type<EqualitySet>::value: {
        auto& f = data.get<EqualitySet>();
        if (f.values[0].is<std::string>()) {
            logMsg("%*s equality set - key:%s val:%s\n", _indent, "",
                   f.key.c_str(),
                   f.values[0].get<std::string>().c_str());
        }
        if (f.values[0].is<double>()) {
            logMsg("%*s equality - key:%s val:%f\n", _indent, "",
                   f.key.c_str(),
                   f.values[0].get<double>());
        }
        break;
    }
    case Data::type<EqualityString>::value: {
        auto& f = data.get<EqualityString>();
            logMsg("%*s equality - key:%s val:%s\n", _indent, "",
                   f.key.c_str(),
                   f.value.c_str());
        break;
    }
    case Data::type<EqualityNumber>::value: {
        auto& f = data.get<EqualityNumber>();
            logMsg("%*s equality - key:%s val:%f\n", _indent, "",
                   f.key.c_str(),
                   f.value);
        break;
    }
    case Data::type<Range>::value: {
        auto& f = data.get<Range>();
        logMsg("%*s range - key:%s min:%f max:%f\n", _indent, "",
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




int Filter::filterCost() const {
    // Add some extra penalty for set vs simple filters
    int sum = 100;

    switch (data.which()) {
    case Data::type<OperatorAny>::value:
        for (auto& f : operands()) { sum += f.filterCost(); }
        return sum;

    case Data::type<OperatorAll>::value:
        for (auto& f : operands()) { sum += f.filterCost(); }
        return sum;

    case Data::type<OperatorNone>::value:
        for (auto& f : operands()) { sum += f.filterCost(); }
        return sum;

    case Data::type<Existence>::value:
        // Equality and Range are more specific for increasing
        // the chance to fail early check them before Existence
        return 20;

    case Data::type<EqualitySet>::value:
    case Data::type<EqualityString>::value:
    case Data::type<EqualityNumber>::value:
    case Data::type<Filter::Range>::value:
        return 1;

    case Data::type<Function>::value:
        // Most expensive filter should be checked last
        return 1000;
    }
    return 0;
}

const std::string& Filter::key() const {
    static const std::string empty = "";

    switch (data.which()) {

    case Data::type<Existence>::value:
        return data.get<Existence>().key;

    case Data::type<EqualitySet>::value:
        return data.get<EqualitySet>().key;

    case Data::type<EqualityString>::value:
        return data.get<EqualityString>().key;

    case Data::type<EqualityNumber>::value:
        return data.get<EqualityNumber>().key;

    case Data::type<Filter::Range>::value:
        return data.get<Range>().key;

    default:
        break;
    }
    return empty;
}

const std::vector<Filter>& Filter::operands() const {
    static const std::vector<Filter> empty;

    switch (data.which()) {
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

const bool Filter::isOperator() const {

    switch (data.which()) {
    case Data::type<OperatorAny>::value:
        return true;

    case Data::type<OperatorAll>::value:
        return true;

    case Data::type<OperatorNone>::value:
        return true;

    default:
        break;
    }
    return false;
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

        if (std::isinf(ra.max) && std::isinf(rb.max)) {
            return rb.min - ra.min;
        }
    }

    return 0;
}


void Filter::sort(std::vector<Filter>& _filters) {

    std::sort(_filters.begin(), _filters.end(),
              [](Filter& a, Filter& b) {

                  // Sort simple filters by eval cost
                  int ma = a.filterCost();
                  int mb = b.filterCost();

                  if (!a.isOperator() && !b.isOperator()) {
                      int diff = ma - mb;
                      if (diff != 0) {
                          return diff < 0;
                      }

                      // just for consistent ordering
                      // (and using > to prefer $zoom over $geom)
                      return a.key() > b.key();
                  }

                  // When one is a simple Filter and the other is a operaor
                  // or both are operators prefer the one with the cheaper
                  // filter(s).
                  if (ma != mb) {
                      return ma < mb;
                  }

                  return compareSetFilter(a, b) < 0;
              });
}


void Filter::collectFilters(Filter& f, FiltersAndKeys& fk) {

    switch (f.data.which()) {

    case Filter::Data::type<Filter::OperatorAny>::value: {
        for (auto& op : f.data.get<Filter::OperatorAny>().operands) {
            collectFilters(op, fk);
        }
        break;
    }
    case Filter::Data::type<Filter::OperatorAll>::value: {
        for (auto& op : f.data.get<Filter::OperatorAll>().operands) {
            collectFilters(op, fk);
        }
        break;
    }
    case Filter::Data::type<Filter::OperatorNone>::value: {
        for (auto& op : f.data.get<Filter::OperatorNone>().operands) {
            collectFilters(op, fk);
        }
        break;
    }
    case Filter::Data::type<Filter::Existence>::value: {
        auto& d = f.data.get<Filter::Existence>();
        fk.keys.push_back(d.key);
        fk.filters.push_back(&f);
        break;
    }
    case Filter::Data::type<Filter::EqualitySet>::value: {
        auto& d = f.data.get<Filter::EqualitySet>();
        if (Filter::keywordType(d.key) == FilterKeyword::undefined) {
            fk.keys.push_back(d.key);
            fk.filters.push_back(&f);
        }
        break;
    }
    case Filter::Data::type<Filter::EqualityString>::value: {
        auto& d = f.data.get<Filter::EqualityString>();
        if (Filter::keywordType(d.key) == FilterKeyword::undefined) {
            fk.keys.push_back(d.key);
            fk.filters.push_back(&f);
        }
        break;
    }
    case Filter::Data::type<Filter::EqualityNumber>::value: {
        auto& d = f.data.get<Filter::EqualityNumber>();
        if (Filter::keywordType(d.key) == FilterKeyword::undefined) {
            fk.keys.push_back(d.key);
            fk.filters.push_back(&f);
        }
        break;
    }
    case Filter::Data::type<Filter::Range>::value: {
        auto& d = f.data.get<Filter::Range>();
        if (Filter::keywordType(d.key) == FilterKeyword::undefined) {
            fk.keys.push_back(d.key);
            fk.filters.push_back(&f);
        }
        break;
    }
    default:
        break;
    }
}

std::vector<std::string> Filter::assignPropertyKeys(FiltersAndKeys& fk) {
    std::sort(fk.keys.begin(), fk.keys.end(), Properties::keyComparator);
    fk.keys.erase(std::unique(fk.keys.begin(), fk.keys.end()), fk.keys.end());
    fk.keys.insert(fk.keys.begin(), "$geometry");
    fk.keys.insert(fk.keys.begin(), "$zoom");

    for (auto& k : fk.keys) {
        LOG("key: %s", k.c_str());
    }

    for (auto* f : fk.filters) {
        std::string key = f->key();
        size_t id = 0;
        for (; id < fk.keys.size(); id++) {
            if (fk.keys[id] == key) { break; }
        }

        switch (f->data.which()) {
        case Filter::Data::type<Filter::Existence>::value:
            f->data.get<Filter::Existence>().keyID = id;
            break;
        case Filter::Data::type<Filter::EqualitySet>::value:
            f->data.get<Filter::EqualitySet>().keyID = id;
            break;
        case Filter::Data::type<Filter::EqualityString>::value:
            f->data.get<Filter::EqualityString>().keyID = id;
            break;
        case Filter::Data::type<Filter::EqualityNumber>::value:
            f->data.get<Filter::EqualityNumber>().keyID = id;
            break;
        case Filter::Data::type<Filter::Range>::value:
            f->data.get<Filter::Range>().keyID = id;
            break;
        default:
            break;

        }
    }
    return fk.keys;
}

struct string_matcher {
    using result_type = bool;
    const std::string& str;

    template <typename T>
    bool operator()(const T& v) const { return false; }
    bool operator()(const std::string& v) const {
        return str == v;
    }
};

struct number_matcher {
    using result_type = bool;
    double num;

    template <typename T>
    bool operator()(const T& v) const { return false; }
    bool operator()(const double& v) const {
        if (num == v) { return true; }
        return std::fabs(num - v) <= std::numeric_limits<double>::epsilon();
    }
};

struct match_equal_set {
    using result_type = bool;
    const std::vector<Value>& values;

    template <typename T>
    bool operator()(const T&) const { return false; }

    bool operator()(const double& num) const {
        number_matcher m{num};
        for (const auto& v : values) {
            if (Value::visit(v, m)) {
                return true;
            }
        }
        return false;
    }
    bool operator()(const std::string& str) const {
        string_matcher m{str};

        for (const auto& v : values) {
            if (Value::visit(v, m)) {
                return true;
            }
        }
        return false;
    }
};

struct match_range {
    const Filter::Range& f;

    bool operator() (const double& num) const {
        return num >= f.min && num < f.max;
    }
    bool operator() (const std::string&) const { return false; }
    bool operator() (const none_type&) const { return false; }
};

struct matcher {
    using result_type = bool;

    matcher(StyleContext& ctx) : ctx(ctx) {}

    StyleContext& ctx;

    bool eval(const Filter::Data& data) const {
        return Filter::Data::visit(data, *this);
    }

    bool operator() (const Filter::OperatorAny& f) const {
        for (const auto& filt : f.operands) {
            if (eval(filt.data)) { return true; }
        }
        return false;
    }
    bool operator() (const Filter::OperatorAll& f) const {
        for (const auto& filt : f.operands) {
            if (!eval(filt.data)) { return false; }
        }
        return true;
    }
    bool operator() (const Filter::OperatorNone& f) const {
        for (const auto& filt : f.operands) {
            if (eval(filt.data)) { return false; }
        }
        return true;
    }
    bool operator() (const Filter::Existence& f) const {
        return f.exists == ctx.hasCachedProperty(f.keyID);
    }
    bool operator() (const Filter::EqualitySet& f) const {
        auto& value = ctx.getCachedProperty(f.keyID);
        return Value::visit(value, match_equal_set{f.values});
    }
    bool operator() (const Filter::EqualityString& f) const {
        auto& value = ctx.getCachedProperty(f.keyID);
        return Value::visit(value, string_matcher{f.value});
    }
    bool operator() (const Filter::EqualityNumber& f) const {
        auto& value = ctx.getCachedProperty(f.keyID);
        return Value::visit(value, number_matcher{f.value});
    }
    bool operator() (const Filter::Range& f) const {
        auto& value = ctx.getCachedProperty(f.keyID);
        return Value::visit(value, match_range{f});
    }
    bool operator() (const Filter::Function& f) const {
        return ctx.evalFilter(f.id);
    }
    bool operator() (const none_type& f) const {
        return true;
    }
};

bool Filter::eval(StyleContext& ctx) const {
    return Data::visit(data, matcher(ctx));
}

}
