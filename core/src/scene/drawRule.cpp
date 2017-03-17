#include "scene/drawRule.h"

#include "drawRuleWarnings.h"
#include "log.h"
#include "platform.h"
#include "scene/scene.h"
#include "scene/sceneLayer.h"
#include "scene/stops.h"
#include "scene/styleContext.h"
#include "style/style.h"
#include "tile/tileBuilder.h"
#include "util/hash.h"

#include <algorithm>

namespace Tangram {

DrawRuleData::DrawRuleData(std::string _name, int _id,
                           std::vector<StyleParam> _parameters)
    : parameters(std::move(_parameters)),
      name(std::move(_name)),
      id(_id) {}

std::string DrawRuleData::toString() const {
    std::string str = "{\n";
    for (auto& p : parameters) {
         str += " { "
             + std::to_string(static_cast<int>(p.key))
             + ", "
             + p.toString()
             + " }\n";
    }
    str += "}\n";

    return str;
}

DrawRule::DrawRule(const DrawRuleData& _ruleData, const std::string& _layerName, size_t _layerDepth) :
    name(&_ruleData.name),
    id(_ruleData.id) {

    for (const auto& param : _ruleData.parameters) {
        auto key = static_cast<uint8_t>(param.key);
        active[key] = true;
        params[key] = { &param, _layerName.c_str(), _layerDepth };
    }
}

void DrawRule::merge(const DrawRuleData& _ruleData, const SceneLayer& _layer) {

    evalConflict(*this, _ruleData, _layer);

    const auto depthNew = _layer.depth();
    const char* layerNew = _layer.name().c_str();

    for (const auto& paramNew : _ruleData.parameters) {

        auto key = static_cast<uint8_t>(paramNew.key);
        auto& param = params[key];

        if (!active[key] || depthNew > param.depth ||
            (depthNew == param.depth && strcmp(layerNew, param.name) > 0)) {
            param = { &paramNew, layerNew, depthNew };
            active[key] = true;
        }
    }
}

bool DrawRule::isJSFunction(StyleParamKey _key) const {
    auto& param = findParameter(_key);
    if (!param) {
        return false;
    }
    return param.function >= 0;
}

bool DrawRule::contains(StyleParamKey _key) const {
    return findParameter(_key) != false;
}

const StyleParam& DrawRule::findParameter(StyleParamKey _key) const {
    static const StyleParam NONE;

    uint8_t key = static_cast<uint8_t>(_key);
    if (!active[key]) { return NONE; }
    return *params[key].param;
}

const std::string& DrawRule::getStyleName() const {

    const auto& style = findParameter(StyleParamKey::style);

    if (style) {
        return style.value.get<std::string>();
    } else {
        return *name;
    }
}

const char* DrawRule::getLayerName(StyleParamKey _key) const {
    return params[static_cast<uint8_t>(_key)].name;
}

size_t DrawRule::getParamSetHash() const {
    size_t seed = 0;
    for (size_t i = 0; i < StyleParamKeySize; i++) {
        if (active[i]) { hash_combine(seed, params[i].name); }
    }
    return seed;
}

void DrawRule::logGetError(StyleParamKey _expectedKey, const StyleParam& _param) const {
    LOGE("wrong type '%d'for StyleParam '%d'", _param.value.which(), _expectedKey);
}

bool DrawRuleMergeSet::match(const Feature& _feature, const SceneLayer& _layer, StyleContext& _ctx) {

    _ctx.setFeature(_feature);
    m_matchedRules.clear();
    m_queuedLayers.clear();

    // If uber layer is marked not visible return immediately
    if (!_layer.visible()) {
        return false;
    }

    // If the first filter doesn't match, return immediately
    if (!_layer.filter().eval(_feature, _ctx)) { return false; }

    m_queuedLayers.push_back(&_layer);

    // Iterate depth-first over the layer hierarchy
    while (!m_queuedLayers.empty()) {

        // Pop a layer off the top of the stack
        const auto& layer = *m_queuedLayers.back();
        m_queuedLayers.pop_back();

        // Merge rules from layer into accumulated set
        mergeRules(layer);

        // Push each of the layer's matching sublayers onto the stack
        for (const auto& sublayer : layer.sublayers()) {
            // Skip matching this sublayer if marked not visible
            if (!sublayer.visible()) {
                continue;
            }

            if (sublayer.filter().eval(_feature, _ctx)) {
                m_queuedLayers.push_back(&sublayer);
            }
        }
    }

    return true;
}

bool DrawRuleMergeSet::evaluateRuleForContext(DrawRule& rule, StyleContext& ctx) {

    bool visible;
    if (rule.get(StyleParamKey::visible, visible) && !visible) {
        return false;
    }

    bool valid = true;
    for (size_t i = 0; i < StyleParamKeySize; ++i) {

        if (!rule.active[i]) {
            rule.params[i].param = nullptr;
            continue;
        }

        auto*& param = rule.params[i].param;

        // Evaluate JS functions and Stops
        if (param->function >= 0) {

            // Copy param into 'evaluated' and point param to the evaluated StyleParam.
            m_evaluated[i] = *param;
            param = &m_evaluated[i];

            if (!ctx.evalStyle(param->function, param->key, m_evaluated[i].value)) {
                if (StyleParam::isRequired(param->key)) {
                    valid = false;
                    break;
                } else {
                    rule.active[i] = false;
                }
            }
        } else if (param->stops) {
            m_evaluated[i] = *param;
            param = &m_evaluated[i];

            Stops::eval(*param->stops, param->key, ctx.getKeywordZoom(), m_evaluated[i].value);
        }
    }

    return valid;
}

void DrawRuleMergeSet::mergeRules(const SceneLayer& _layer) {

    size_t pos, end = m_matchedRules.size();

    for (const auto& rule : _layer.rules()) {
        for (pos = 0; pos < end; pos++) {
            if (m_matchedRules[pos].id == rule.id) { break; }
        }

        if (pos == end) {
            m_matchedRules.emplace_back(rule, _layer.name(), _layer.depth());
        } else {
            m_matchedRules[pos].merge(rule, _layer);
        }
    }
}

}
