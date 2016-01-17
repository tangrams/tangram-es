#include "drawRule.h"

#include "tile/tile.h"
#include "scene/scene.h"
#include "scene/sceneLayer.h"
#include "scene/stops.h"
#include "scene/styleContext.h"
#include "style/style.h"
#include "platform.h"
#include "drawRuleWarnings.h"

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

DrawRule::DrawRule(const DrawRuleData& _ruleData) :
    name(&_ruleData.name),
    id(_ruleData.id) {
}

void DrawRule::merge(const DrawRuleData& _ruleData, const SceneLayer& _layer) {
    // TODO Optimize - string copying going on here

    evalConflict(*this, _ruleData, _layer);

    const auto depthNew = _layer.depth();
    const char* layerNew = _layer.name().c_str();

    for (const auto& param : _ruleData.parameters) {

        auto key = static_cast<uint8_t>(param.key);
        auto& layer = layers[key];

        if (depthNew > layer.depth || layer.name == nullptr ||
            (depthNew == layer.depth && strcmp(layerNew, layer.name) > 0)) {
            params[key] = &param;
            layer.name = layerNew;
            layer.depth = depthNew;
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

    const auto* p = params[static_cast<uint8_t>(_key)];
    if (p) {
        return *p;
    }

    return NONE;
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
    return layers[static_cast<uint8_t>(_key)];
}

std::set<const char*> DrawRule::getLayerNames() const {
    std::set<const char*> layerNames;

    for (size_t i = 0; i < StyleParamKeySize; i++) {
        if (layers[i][0] != '\0') {
            layerNames.insert(layers[i]);
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

            auto* param = rule.params[i];
            if (!param) { continue; }

            if (param->function >= 0) {

                if (!_ctx.evalStyle(param->function, param->key, m_evaluated[i].value) &&
                    StyleParam::isRequired(param->key)) {
                    valid = false;
                    break;
                }

                m_evaluated[i].function = param->function;
                rule.params[i] = &m_evaluated[i];

            }
            if (param->stops) {

                m_evaluated[i].stops = param->stops;

                if (StyleParam::isColor(param->key)) {
                    m_evaluated[i].value = param->stops->evalColor(_ctx.getGlobalZoom());
                } else if (StyleParam::isWidth(param->key)) {
                    // FIXME width result is ignored from here
                    m_evaluated[i].value = param->stops->evalWidth(_ctx.getGlobalZoom());
                } else if (StyleParam::isOffsets(param->key)) {
                    m_evaluated[i].value = param->stops->evalVec2(_ctx.getGlobalZoom());
                } else {
                    m_evaluated[i].value = param->stops->evalFloat(_ctx.getGlobalZoom());
                }

                rule.params[i] = &m_evaluated[i];

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
                               [&](auto& m) { return rule.id == m.id; });

        if (it == m_matchedRules.end()) {
            it = m_matchedRules.emplace(it, rule);
        }

        it->merge(rule, _layer);
    }
}

}
