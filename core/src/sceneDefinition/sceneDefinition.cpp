#include "sceneDefinition.h"

void SceneDefinition::addStyle(std::unique_ptr<Style> _style) {

    m_styles.push_back(std::move(_style));

}

