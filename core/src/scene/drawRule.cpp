#include "drawRule.h"

#include "tile/tile.h"
#include "scene/scene.h"
#include "scene/sceneLayer.h"
#include "scene/stops.h"
#include "scene/styleContext.h"
#include "style/style.h"
#include "platform.h"

#include <algorithm>

namespace Tangram {

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
        return styleName;
    }
}

void DrawRule::logGetError(StyleParamKey _expectedKey, const StyleParam& _param) const {
    LOGE("wrong type '%d'for StyleParam '%d'", _param.value.which(), _expectedKey);
}

void Styling::apply(Tile& _tile, const Feature& _feature,
                    const Scene& _scene, StyleContext& _ctx) {

    for (auto& rule : styles) {

        auto& styleName = rule.getStyleName();
        int styleId = styleName.empty()
            ? rule.styleId
            : _scene.getStyleId(styleName);

        if (styleId < 0){
            LOGE("Invalid style %s", rule.getStyleName().c_str());
            continue;
        }
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
            auto& style = _scene.styles()[styleId];
            style->buildFeature(_tile, _feature, rule);
        }
    }
}

void Styling::mergeRules(const std::vector<StaticDrawRule>& rules) {
    for (auto& rule : rules) {

        auto it = std::find_if(styles.begin(), styles.end(), [&](auto& m) {
                return rule.styleId == m.styleId; });

        if (it == styles.end()) {
            styles.emplace_back();
            it = styles.end()-1;
            it->styleId = rule.styleId;
            it->styleName = rule.styleName;
        }

        for (auto& param : rule.parameters) {
            it->params[static_cast<uint8_t>(param.key)] = &param;
        }
    }
}

}
