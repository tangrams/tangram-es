#include "scene/filters.h"

#include "data/tileData.h"
#include "log.h"
#include "platform.h"
#include "scene/styleContext.h"
#include "scene/scene.h"
#include "util/floatFormatter.h"
#include "util/yamlUtil.h"

#include "yaml-cpp/yaml.h"

#include <cmath>

#define LOGNode(fmt, node, ...) LOGW(fmt ":\n'%s'\n", ## __VA_ARGS__, YAML::Dump(node).c_str())

namespace Tangram {

const std::string Filter::key_geom{"$geometry"};
const std::string Filter::key_zoom{"$zoom"};
const std::string Filter::key_other{""};

const std::vector<std::string> Filter::geometryStrings = { "", "point", "line", "polygon" };


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
    case Data::type<EqualityStringSet>::value: {
        auto& f = data.get<EqualityStringSet>();
        for (auto& val : data.get<EqualityStringSet>().values) {
            logMsg("%*s equality set - key:%s val:%s\n", _indent, "",
                   f.key.c_str(), val.c_str());
        }
        break;
    }
    case Data::type<EqualityNumberSet>::value: {
        auto& f = data.get<EqualityNumberSet>();
        for (auto& val : data.get<EqualityNumberSet>().values) {
            logMsg("%*s equality set - key:%f val:%f\n", _indent, "",
                   f.key.c_str(), val);
        }
        break;
    }
    case Data::type<EqualityString>::value: {
        auto& f = data.get<EqualityString>();
        logMsg("%*s equality - key:%s val:%s\n", _indent, "",
               f.key.c_str(), f.value.c_str());
        break;
    }
    case Data::type<EqualityNumber>::value: {
        auto& f = data.get<EqualityNumber>();
        logMsg("%*s equality - key:%s val:%f\n", _indent, "",
               f.key.c_str(), f.value);
        break;
    }
    case Data::type<Range>::value: {
        auto& f = data.get<Range>();
        logMsg("%*s range - key:%s min:%f max:%f\n", _indent, "",
               f.key.c_str(), f.min, f.max);
        return;
    }
    case Data::type<RangeKeyZoom>::value: {
        auto& f = data.get<RangeKeyZoom>();
        logMsg("%*s range - key:%s min:%d max:%d\n", _indent, "",
               keyName(f.key).c_str(), f.min, f.max);
        return;
    }
    case Data::type<EqualityKey>::value: {
        auto& f = data.get<EqualityKey>();
        logMsg("%*s equality - key:%s val:%d\n", _indent, "",
               keyName(f.key).c_str(), f.value);
        break;
    }
    case Data::type<Function>::value: {
        logMsg("%*s function\n", _indent, "");
        break;
    }
    default:
        logMsg("missing!\n");
        break;
    }
}


int Filter::filterCost() const {
    // Add some extra penalty for set vs simple filters
    int sum = 100;

    switch (data.which()) {
    case Data::type<OperatorAny>::value:
        for (auto& f : operands()) { sum += f.filterCost(); }
        return sum * operands().size();

    case Data::type<OperatorAll>::value:
        for (auto& f : operands()) { sum += f.filterCost(); }
        return sum * operands().size();

    case Data::type<OperatorNone>::value:
        for (auto& f : operands()) { sum += f.filterCost(); }
        return sum * operands().size();

    case Data::type<EqualityStringSet>::value:
        return 5 * data.get<EqualityStringSet>().values.size();

    case Data::type<EqualityNumberSet>::value:
        return 5 * data.get<EqualityNumberSet>().values.size();

    case Data::type<EqualityString>::value:
        return 10;

    case Data::type<EqualityNumber>::value:
        return 10;

    case Data::type<Filter::Range>::value:
        return 10;

    case Data::type<Filter::RangeArea>::value:
        return 10;

    case Data::type<Existence>::value:
        return 5;

    case Data::type<EqualityKey>::value:
        return 1;

    case Data::type<Filter::RangeKeyZoom>::value:
        return 1;

    case Data::type<Function>::value:
        // Most expensive filter should be checked last
        // - unless it is most specific, e.g. global false..
        return 1000;
    }
    return 0;
}

const std::string& Filter::key() const {
    static const std::string empty = "";

    switch (data.which()) {

    case Data::type<Existence>::value:
        return data.get<Existence>().key;

    // case Data::type<EqualityKeySet>::value:
    //     return keyName(data.get<EqualityKeySet>().key);

    case Data::type<EqualityNumberSet>::value:
        return data.get<EqualityNumberSet>().key;

    case Data::type<EqualityStringSet>::value:
        return data.get<EqualityStringSet>().key;

    case Data::type<EqualityKey>::value:
        return keyName(data.get<EqualityKey>().key);

    case Data::type<EqualityString>::value:
        return data.get<EqualityString>().key;

    case Data::type<EqualityNumber>::value:
        return data.get<EqualityNumber>().key;

    case Data::type<Filter::Range>::value:
        return data.get<Range>().key;

    case Data::type<Filter::RangeArea>::value:
        return data.get<RangeArea>().key;

    case Data::type<Filter::RangeKeyZoom>::value:
        return key_zoom;

    default:
        break;
    }
    return key_other;
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

bool Filter::isOperator() const {

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

int Filter::compareSetFilter(const Filter& a, const Filter& b) {
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
    std::sort(_filters.begin(), _filters.end(), [](Filter& a, Filter& b) {
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

struct Filter::matcher {
    using result_type = bool;

    matcher(const Feature& feat, StyleContext& ctx) :
        props(feat.props), ctx(ctx) {}

    const Properties& props;
    StyleContext& ctx;

    bool eval(const Filter::Data& data) const {
        return Filter::Data::visit(data, *this);
    }

    bool operator() (const Filter::EqualityKey& f) const {
        return f.value == ctx.getFilterKey(f.key);
    }

    bool operator() (const Filter::EqualityString& f) const {
        auto& propValue = props.get(f.key);

        if (propValue.is<std::string>()) {
            auto& str = propValue.get<std::string>();
            return str == f.value;
        }
        return false;
    }

    bool operator() (const Filter::EqualityNumber& f) const {
        auto& propValue = props.get(f.key);
        if (propValue.is<double>()) {
            double num = propValue.get<double>();
            if (num == f.value) { return true; }
            return std::fabs(num - f.value) <= std::numeric_limits<double>::epsilon();
        }
        return false;
    }

    bool operator() (const Filter::RangeKeyZoom& f) const {
        float num = ctx.getZoomLevel();
        return num >= f.min && num < f.max;
    }

    bool operator() (const Filter::Range& f) const {
        auto& propValue = props.get(f.key);
        if (!propValue.is<double>()) return false;

        double num = propValue.get<double>();
        return num >= f.min && num < f.max;
    }

    bool operator() (const Filter::RangeArea& f) const {
        auto& propValue = props.get(f.key);
        if (!propValue.is<double>()) return false;

        float scale = ctx.getPixelAreaScale();

        double num = propValue.get<double>();
        return num >= f.min * scale && num < f.max * scale;
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
        return f.exists == props.contains(f.key);
    }

    bool operator() (const Filter::EqualityNumberSet& f) const {
        auto& propValue = props.get(f.key);
        if (!propValue.is<double>()) {
            return false;
        }
        double num = propValue.get<double>();
        for (auto& value : f.values) {
            if (num == value || std::fabs(num - value) <= std::numeric_limits<double>::epsilon()) {
                return true;
            }
        }
        return false;
    }

    bool operator() (const Filter::EqualityStringSet& f) const {
        auto& propValue = props.get(f.key);
        if (!propValue.is<std::string>()) {
            return false;
        }
        auto& str = propValue.get<std::string>();
        for (auto& value : f.values) {
            if (str == value) { return true;}
        }
        return false;
    }

    bool operator() (const Filter::Function& f) const {
        return ctx.evalFilter(f.id);
    }

    bool operator() (const none_type& f) const {
        return true;
    }
};

bool Filter::eval(const Feature& feat, StyleContext& ctx) const {
    return Data::visit(data, matcher(feat, ctx));
}




Filter Filter::getEqualityStringFilter(const std::string& k, const std::vector<std::string>& vals) {
    auto key = keyType(k);
    if (key == Key::other) {
        if (vals.size() == 1) {
            return Filter{ EqualityString { k, vals[0] }};
        } else {
            return Filter{ EqualityStringSet{ k, vals }};
        }
    }
    auto checkGeomType = [](const std::string& val, int& out) {
        for (int i = 1; i < geometryStrings.size(); i++) {
            if (val == geometryStrings[i]) {
                out = i;
                return true;
            }
        }
        return false;
    };

    if (key == Filter::Key::geometry) {
        std::vector<Filter> geoms;
        for (auto& val : vals) {
            int geomType;
            if (checkGeomType(val, geomType)) {
                geoms.emplace_back(Filter::EqualityKey{ Key::geometry, geomType });
            } else {
                //LOG(invalid);
            }
        }
        if (geoms.empty()) {
            // LOG(invalid)
            return {};
        }
        if (geoms.size() == 1) {
            return std::move(geoms[0]);
        }
        return Filter{ OperatorAny{ std::move(geoms) }};
    }
    // LOG(invalid)
    return {};
}

Filter Filter::getEqualityNumberFilter(const std::string& k, const std::vector<double>& vals) {
    auto key = keyType(k);
    if (key == Key::other) {
        if (vals.size() == 1) {
            return Filter{ EqualityNumber { k, vals[0] }};
        } else {
            return Filter{ EqualityNumberSet { k, vals }};
        }
    }
    if (key == Key::zoom) {
        // TODO EqualityKeySet

        // TODO could also LOG negative zooms...
        if (vals.size() == 1) {
            return Filter{ EqualityKey{ Key::zoom, int(vals[0]) } };
        }
        std::vector<Filter> zooms;

        for (auto& val : vals) {
            int ival = int(val);
            if (val > std::numeric_limits<int>::max()) {
                ival = std::numeric_limits<int>::max();
            }
            if (ival < 0) { ival = 0; }
            zooms.emplace_back(Filter::EqualityKey{ Key::zoom, ival});
        }
        return Filter{ OperatorAny{ std::move(zooms) }};
    }
    return {};
}

Filter Filter::getRangeFilter(const std::string& k, float min, float max, bool sqArea) {
    auto key = keyType(k);
    if (key == Key::other) {
        if (sqArea) {
            return Filter{ RangeArea{ k, min, max }};
        } else {
            return Filter{ Range{ k, min, max }};
        }
    }
    if (key == Key::zoom) {
        int imax = int(max);
        if (imax < 0) { imax = std::numeric_limits<int>::max(); }
        int imin = int(min);
        if (imin < 0) { imin = 0; }
        return Filter{ RangeKeyZoom{ key, imin, imax }};
    }
    return {};
}

Filter Filter::generateFilter(const YAML::Node& _filter, SceneFunctions& _fns) {

    switch (_filter.Type()) {
    case YAML::NodeType::Scalar: {

        const std::string& val = _filter.Scalar();
        if (val.compare(0, 8, "function") == 0) {
            return Filter{Function{uint32_t(_fns.add(val))}};
        }
        LOGNode("invalid", _filter);
        return {};
    }
    case YAML::NodeType::Sequence: {
        return generateAnyFilter(_filter, _fns);
    }
    case YAML::NodeType::Map: {
        std::vector<Filter> filters;
        for (const auto& filtItr : _filter) {
            const std::string& key = filtItr.first.Scalar();
            const YAML::Node& node = _filter[key];
            Filter&& f = {};
            if (key == "none") {
                f = generateNoneFilter(node, _fns);
            } else if (key == "not") {
                f = generateNoneFilter(node, _fns);
            } else if (key == "any") {
                f = generateAnyFilter(node, _fns);
            } else if (key == "all") {
                f = generateAllFilter(node, _fns);
            } else {
                f = generatePredicate(node, key);
            }
            if (f.isValid()) { filters.push_back(std::move(f)); }
        }
        if (!filters.empty()) {
            if (filters.size() == 1) {
                return std::move(filters.front());
            }
            sort(filters);
            return Filter{OperatorAll{std::move(filters)}};
        }
    }
    default:
        break;
    }
    LOGNode("invalid", _filter);
    return {};

}

Filter Filter::generatePredicate(const YAML::Node& _value, std::string _key) {

    switch (_value.Type()) {
    case YAML::NodeType::Scalar: {
        if (_value.Tag() == "tag:yaml.org,2002:str") {
            // Node was explicitly tagged with '!!str' or the canonical tag
            // 'tag:yaml.org,2002:str' yaml-cpp normalizes the tag value to the
            // canonical form
            return getEqualityStringFilter(_key, { _value.Scalar() });
        }
        double number;
        if (YamlUtil::getDouble(_value, number, false)) {
            return getEqualityNumberFilter(_key, { number });
        }
        bool existence;
        if (YamlUtil::getBool(_value, existence)) {
            return Filter{Existence{_key, existence}};
        }
        return getEqualityStringFilter(_key, { _value.Scalar() });
    }
    case YAML::NodeType::Sequence: {
        std::vector<std::string> strings;
        std::vector<double> numbers;

        for (const auto& valItr : _value) {
            double number;
            // Don't allow mixing type - does not make sense
            if (YamlUtil::getDouble(valItr, number, false)) {
                if (strings.empty()) {
                    numbers.emplace_back(number);
                } else {
                    LOGNode("invalid, key:%s", _value, _key.c_str());
                }
            } else {
                if (numbers.empty()) {
                    strings.push_back(valItr.Scalar());
                } else {
                    LOGNode("invalid, key:%s", _value, _key.c_str());
                }
            }
        }
        if (!strings.empty()) {
            return getEqualityStringFilter(_key, std::move(strings));
        }
        if (!numbers.empty()) {
            return getEqualityNumberFilter(_key, std::move(numbers));
        }
        break;
    }
    case YAML::NodeType::Map: {
        double minVal = -std::numeric_limits<double>::infinity();
        double maxVal = std::numeric_limits<double>::infinity();
        bool hasMinPixelArea = false;
        bool hasMaxPixelArea = false;
        bool hasMin = false;
        bool hasMax = false;

        for (const auto& n : _value) {
            if (n.first.Scalar() == "min") {
                hasMin = getFilterRangeValue(n.second, minVal, hasMinPixelArea);
                continue;
            } else if (n.first.Scalar() == "max") {
                hasMax = getFilterRangeValue(n.second, maxVal, hasMaxPixelArea);
                continue;
            }
            LOGNode("invalid, key:%s", _value, _key.c_str());
            return {};
        }

        if (hasMin && hasMax && hasMinPixelArea != hasMaxPixelArea) {
            LOGNode("invalid, key:%s", _value, _key.c_str());
            return {};
        }
        return getRangeFilter(_key, minVal, maxVal, hasMinPixelArea);
    }
    default:
        break;
    }

    LOGNode("invalid, key:%s", _value, _key.c_str());
    return {};
}

bool Filter::getFilterRangeValue(const YAML::Node& node, double& val, bool& hasPixelArea) {
    if (!YamlUtil::getDouble(node, val, false)) {
        auto strVal = node.Scalar();
        auto n = strVal.find("px2");
        if (n == std::string::npos) { return false; }
        try {
            val = ff::stof(std::string(strVal, 0, n));
            hasPixelArea = true;
        } catch (std::invalid_argument) { return false; }
    }
    return true;
}

Filter Filter::generateAnyFilter(const YAML::Node& _filter, SceneFunctions& _fns) {

    if (!_filter.IsSequence()) {
        LOGNode("invalid any filter", _filter);
        return {};
    }
    std::vector<Filter> filters;

    for (const auto& filt : _filter) {
        if (Filter f = generateFilter(filt, _fns)) {
            if (f.data.is<OperatorAny>()) {
                // flatten filter
                for (auto& ff : f.data.get<OperatorAny>().operands) {
                    filters.push_back(std::move(ff));
                }
            } else {
                filters.push_back(std::move(f));
            }
        } else {
            LOGNode("invalid any filter", _filter);
            continue;
        }
    }

    if (filters.empty()) {
        LOGNode("invalid any filter", _filter);
        return {};
    }
    if (filters.size() == 1) {
        LOGNode("useless any filter", _filter);
        return std::move(filters.front());
    }
    sort(filters);
    return Filter{OperatorAny{std::move(filters)}};
}

Filter Filter::generateAllFilter(const YAML::Node& _filter, SceneFunctions& _fns) {

    if (!_filter.IsSequence()) {
        LOGNode("invalid all filter", _filter);
        return {};
    }

    std::vector<Filter> filters;

    for (const auto& filt : _filter) {
        if (Filter f = generateFilter(filt, _fns)) {
            if (f.data.is<OperatorAll>()) {
                // flatten filter
                for (auto& ff : f.data.get<OperatorAll>().operands) {
                    filters.push_back(std::move(ff));
                }
            } else {
                filters.push_back(std::move(f));
            }
        } else {
            LOGNode("invalid all filter", _filter);
        }
    }
    if (filters.empty()) {
        LOGNode("invalid all filter", _filter);
        return {};
    }
    if (filters.size() == 1) {
        LOGNode("useless all filter", _filter);
        return std::move(filters.front());
    }
    sort(filters);
    return Filter{OperatorAll{std::move(filters)}};
}

Filter Filter::generateNoneFilter(const YAML::Node& _filter, SceneFunctions& _fns) {

    if (_filter.IsMap() || _filter.IsScalar()) {
        // 'not' case
        if (Filter&& f = generateFilter(_filter, _fns)) {
            return Filter{OperatorNone{{std::move(f)}}};
        }
        LOGNode("invalid 'none' filter", _filter);
        return {};
    }
    if (!_filter.IsSequence()) {
        LOGNode("invalid 'none' filter", _filter);
        return {};
    }
    std::vector<Filter> filters;
    for (const auto& filt : _filter) {
        if (Filter&& f = generateFilter(filt, _fns)) {
            if (f.data.is<OperatorNone>()) {
                // flatten filter
                for (auto& ff : f.data.get<OperatorNone>().operands) {
                    filters.push_back(std::move(ff));
                }
            } else {
                filters.push_back(std::move(f));
            }

        } else {
            LOGNode("invalid 'none' filter", _filter);
        }
    }
    if (filters.empty()) {
        LOGNode("invalid 'none' filter", _filter);
        return {};
    }
    if (filters.size() == 1) {
        LOGNode("useless 'none' filter", _filter);
        return std::move(filters.front());
    }
    sort(filters);
    return Filter{OperatorNone{std::move(filters)}};
}

}
