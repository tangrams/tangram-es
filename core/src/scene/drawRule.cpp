#include "scene/drawRule.h"

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

DrawRule::DrawRule(const DrawRuleData& ruleData, const std::string& layerName, int layerDepth) :
    name(&ruleData.name),
    id(ruleData.id) {

    for (const auto& param : ruleData.parameters) {
        auto key = static_cast<uint8_t>(param.key);
        active[key] = true;
        params[key] = { &param, layerName.c_str(), layerDepth };
    }
}

bool DrawRule::hasParameterSet(StyleParamKey _key) const {
    if (auto& param = findParameter(_key)) {
        auto key = static_cast<uint8_t>(param.key);
        return active[key];
    }
    return false;
}

void DrawRule::merge(const DrawRuleData& ruleData, const std::string& layerName, int layerDepth) {

    for (const auto& paramNew : ruleData.parameters) {

        auto key = static_cast<uint8_t>(paramNew.key);
        auto& param = params[key];

        if (!active[key] || layerDepth > param.layerDepth) {
            param = { &paramNew, layerName.c_str(), layerDepth };
            active[key] = true;
        }
    }
}

bool DrawRule::contains(StyleParamKey _key) const {
    return findParameter(_key) != false;
}

const StyleParam& DrawRule::findParameter(StyleParamKey _key) const {
    static const StyleParam NONE;

    auto key = static_cast<uint8_t>(_key);
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

size_t DrawRule::getParamSetHash() const {
    size_t seed = 0;
    for (size_t i = 0; i < StyleParamKeySize; i++) {
        if (active[i]) { hash_combine(seed, params[i].layerName); }
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
    if (!_layer.enabled()) {
        return false;
    }

    // If the first filter doesn't match, return immediately
    if (!_layer.filter().eval(_feature, _ctx)) { return false; }

    m_queuedLayers.push_back({ &_layer, 1 });

    // Iterate depth-first over the layer hierarchy
    while (!m_queuedLayers.empty()) {

        // Pop a layer off the top of the stack
        const auto& layer = *m_queuedLayers.back().layer;
        const auto depth = m_queuedLayers.back().depth;
        m_queuedLayers.pop_back();

        // Merge rules from layer into accumulated set
        mergeRules(layer, depth);

        // Push each of the layer's matching sublayers onto the stack
        for (const auto& sublayer : layer.sublayers()) {
            // Skip matching this sublayer if marked not visible
            if (!sublayer.enabled()) {
                continue;
            }

            if (sublayer.filter().eval(_feature, _ctx)) {
                m_queuedLayers.push_back({ &sublayer, depth + 1 });
                if (sublayer.exclusive()) {
                    break;
                }
            }
        }
    }

    return true;
}

bool DrawRuleMergeSet::evaluateRuleForContext(DrawRule& rule, StyleContext& context) {

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

            if (!context.evalStyle(param->function, param->key, m_evaluated[i].value)) {
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

            Stops::eval(*param->stops, param->key, context.getZoom(), m_evaluated[i].value);
        }
    }

    return valid;
}

void DrawRuleMergeSet::mergeRules(const SceneLayer& layer, int depth) {

    size_t pos, end = m_matchedRules.size();

    for (const auto& rule : layer.rules()) {
        for (pos = 0; pos < end; pos++) {
            if (m_matchedRules[pos].id == rule.id) { break; }
        }

        if (pos == end) {
            m_matchedRules.emplace_back(rule, layer.name(), depth);
        } else {
            m_matchedRules[pos].merge(rule, layer.name(), depth);
        }
    }
}

}
