#include "sceneDefinition.h"

void SceneDefinition::addStyle(Style&& _style) {

    m_styles.push_back(std::unique_ptr<Style>(_style));

}