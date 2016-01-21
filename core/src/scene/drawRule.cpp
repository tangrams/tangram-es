#include "drawRule.h"

#include "tile/tile.h"
#include "scene/scene.h"
#include "scene/sceneLayer.h"
#include "scene/stops.h"
#include "scene/styleContext.h"
#include "style/style.h"
#include "platform.h"
#include "drawRuleWarnings.h"
#include "util/hash.h"

#include <algorithm>

namespace Tangram {

DrawRuleData::DrawRuleData(std::string _name, int _id,
                           const std::vector<StyleParam>& _parameters) :
    parameters(_parameters),
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

DrawRule::DrawRule(const DrawRuleData& _ruleData, const SceneLayer& _layer) :
    styleName(&_ruleData.name),
    styleId(_ruleData.id) {

    const char* layerName = _layer.name().c_str();
    const auto layerDepth = _layer.depth();

    for (const auto& param : _ruleData.parameters) {
        auto key = static_cast<uint8_t>(param.key);
        active[key] = true;
        params[key] = &param;
        layers[key] = { layerName, layerDepth };
    }
}

void DrawRule::merge(const DrawRuleData& _ruleData, const SceneLayer& _layer) {

    evalConflict(*this, _ruleData, _layer);

    const auto depthNew = _layer.depth();
    const char* layerNew = _layer.name().c_str();

    for (const auto& paramNew : _ruleData.parameters) {

        auto key = static_cast<uint8_t>(paramNew.key);
        auto& param = params[key];
        auto& layer = layers[key];

        if (!active[key] || depthNew > layer.depth ||
            (depthNew == layer.depth && strcmp(layerNew, layer.name) > 0)) {
            param = &paramNew;
            layer.name = layerNew;
            layer.depth = depthNew;
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
    return *params[key];
}

const std::string& DrawRule::getStyleName() const {

    const auto& style = findParameter(StyleParamKey::style);

    if (style) {
        return style.value.get<std::string>();
    } else {
        return *styleName;
    }
}

const char* DrawRule::getLayerName(StyleParamKey _key) const {
    return layers[static_cast<uint8_t>(_key)].name;
}

std::set<const char*> DrawRule::getLayerNames() const {
    std::set<const char*> layerNames;

    for (size_t i = 0; i < StyleParamKeySize; i++) {
        if (layers[i].name) {
            layerNames.insert(layers[i].name);
        }
    }
    return layerNames;
}

void DrawRule::logGetError(StyleParamKey _expectedKey, const StyleParam& _param) const {
    LOGE("wrong type '%d'for StyleParam '%d'", _param.value.which(), _expectedKey);
}

bool DrawRuleMergeSet::match(const Feature& _feature, const SceneLayer& _layer, StyleContext& _ctx) {

    _ctx.setFeature(_feature);
    m_matchedRules.clear();
    m_queuedLayers.clear();

    // If the first filter doesn't match, return immediately
    if (!_layer.filter().eval(_feature, _ctx)) { return false; }

    // Add the first layer to the stack
    m_queuedLayers.push_back(&_layer);

    // Iterate depth-first over the layer hierarchy
    while (!m_queuedLayers.empty()) {

        // Pop a layer off the top of the stack
        const auto& layer = *m_queuedLayers.back();
        m_queuedLayers.pop_back();

        // If the filter doesn't pass, move on to the next one
        if (!layer.filter().eval(_feature, _ctx)) { continue; }

        // Push each of the layer's sublayers onto the stack
        for (const auto& sublayer : layer.sublayers()) {
            m_queuedLayers.push_back(&sublayer);
        }

        // Merge rules from current layer into accumulated set
        mergeRules(layer);
    }
    return true;
}

void DrawRuleMergeSet::apply(const Feature& _feature, const Scene& _scene, const SceneLayer& _layer,
                    StyleContext& _ctx, Tile& _tile) {

    // If no rules matched the feature, return immediately
    if (!match(_feature, _layer, _ctx)) { return; }

    // For each matched rule, find the style to be used and
    // build the feature with the rule's parameters
    for (auto& rule : m_matchedRules) {

        auto* style = _scene.findStyle(rule.getStyleName());
        if (!style) {
            LOGE("Invalid style %s", rule.getStyleName().c_str());
            continue;
        }

        bool visible;
        if (rule.get(StyleParamKey::visible, visible) && !visible) {
            continue;
        }

        // Evaluate JS functions and Stops
        bool valid = true;
        for (size_t i = 0; i < StyleParamKeySize; ++i) {

            if (!rule.active[i]) { continue; }

            auto*& param = rule.params[i];
            if (param->function >= 0) {

                if (!_ctx.evalStyle(param->function, param->key, m_evaluated[i].value) &&
                    StyleParam::isRequired(param->key)) {
                    valid = false;
                    break;
                }
                m_evaluated[i].function = param->function;
                param = &m_evaluated[i];
            }
            if (param->stops) {

                m_evaluated[i].stops = param->stops;

                if (StyleParam::isColor(param->key)) {
                    m_evaluated[i].value = param->stops->evalColor(_ctx.getGlobalZoom());
                } else if (StyleParam::isWidth(param->key)) {
                    m_evaluated[i].value = param->stops->evalWidth(_ctx.getGlobalZoom());
                } else if (StyleParam::isOffsets(param->key)) {
                    m_evaluated[i].value = param->stops->evalVec2(_ctx.getGlobalZoom());
                } else {
                    m_evaluated[i].value = param->stops->evalFloat(_ctx.getGlobalZoom());
                }
                param = &m_evaluated[i];
            }
        }

        if (valid) {
            style->buildFeature(_tile, _feature, rule);
        }
    }
}

void DrawRuleMergeSet::mergeRules(const SceneLayer& _layer) {

    for (const auto& rule : _layer.rules()) {

        auto it = std::find_if(m_matchedRules.begin(), m_matchedRules.end(),
                               [&](auto& m) { return rule.id == m.styleId; });

        if (it == m_matchedRules.end()) {
            m_matchedRules.emplace_back(rule, _layer);
        } else {
            it->merge(rule, _layer);
        }
    }
}

}
