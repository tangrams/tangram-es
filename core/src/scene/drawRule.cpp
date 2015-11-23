#include "drawRule.h"

#include "tile/tile.h"
#include "scene/scene.h"
#include "scene/sceneLayer.h"
#include "scene/stops.h"
#include "scene/styleContext.h"
#include "style/style.h"
#include "platform.h"

#include <algorithm>
#include <deque>

namespace Tangram {

DrawRule::DrawRule(const StaticDrawRule& _rule) :
    name(&_rule.name),
    id(_rule.id) {}

const StyleParam& DrawRule::findParameter(StyleParamKey _key) const {
    static const StyleParam NONE;

    const auto* p = params[static_cast<uint8_t>(_key)];
    if (p) {
        if (p->function >= 0 || p->stops != nullptr) {
            return evaluated[static_cast<uint8_t>(_key)];
        }
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

void DrawRule::logGetError(StyleParamKey _expectedKey, const StyleParam& _param) const {
    LOGE("wrong type '%d'for StyleParam '%d'", _param.value.which(), _expectedKey);
}

bool Styling::match(const Feature& _feature, const SceneLayer& _layer, StyleContext& _ctx) {

    _ctx.setFeature(_feature);
    matchedRules.clear();
    queuedLayers.clear();

    // Match layers
    if (!_layer.filter().eval(_feature, _ctx)) { return false; }

    // Add initial drawrules
    mergeRules(_layer.rules());

    const auto& sublayers = _layer.sublayers();
    for (auto it = sublayers.begin(); it != sublayers.end(); ++it) {
        queuedLayers.push_back(it);
    }

    while (!queuedLayers.empty()) {
        const auto& layer = *queuedLayers.front();
        queuedLayers.pop_front();

        if (!layer.filter().eval(_feature, _ctx)) { continue; }

        const auto& sublayers = layer.sublayers();
        for (auto it = sublayers.begin(); it != sublayers.end(); ++it) {
            queuedLayers.push_back(it);
        }
        // override with sublayer drawrules
        mergeRules(layer.rules());
    }
    return true;
}

void Styling::apply(const Feature& _feature, const Scene& _scene, const SceneLayer& _layer,
                    StyleContext& _ctx, Tile& _tile) {

    if (!match(_feature, _layer, _ctx)) { return; }

    // Apply styles
    for (auto& rule : matchedRules) {

        auto* style = _scene.findStyle(rule.getStyleName());

        if (!style) {
            LOGE("Invalid style %s", rule.getStyleName().c_str());
            continue;
        }

        // Evaluate JS functions and Stops
        bool valid = true;
        for (size_t i = 0; i < StyleParamKeySize; ++i) {
            auto* param = rule.params[i];
            if (!param) { continue; }

            if (param->function >= 0) {
                if (!_ctx.evalStyle(param->function, param->key, rule.evaluated[i].value) &&
                    StyleParam::isRequired(param->key)) {
                    valid = false;
                    break;
                }
            }
            if (param->stops) {
                rule.evaluated[i].stops = param->stops;

                if (StyleParam::isColor(param->key)) {
                    rule.evaluated[i].value = param->stops->evalColor(_ctx.getGlobalZoom());
                } else if (StyleParam::isWidth(param->key)) {
                    // FIXME widht result is isgnored from here..
                    rule.evaluated[i].value = param->stops->evalWidth(_ctx.getGlobalZoom());
                } else {
                    rule.evaluated[i].value = param->stops->evalFloat(_ctx.getGlobalZoom());
                }
            }
            rule.evaluated[i].key = param->key;
        }

        if (valid) {
            style->buildFeature(_tile, _feature, rule);
        }
    }
}

void Styling::mergeRules(const std::vector<StaticDrawRule>& rules) {
    for (auto& rule : rules) {

        auto it = std::find_if(matchedRules.begin(), matchedRules.end(),
                               [&](auto& m) { return rule.id == m.id; });

        if (it == matchedRules.end()) {
            it = matchedRules.insert(it, DrawRule(rule));
        }

        for (auto& param : rule.parameters) {
            it->params[static_cast<uint8_t>(param.key)] = &param;
        }
    }
}

}
