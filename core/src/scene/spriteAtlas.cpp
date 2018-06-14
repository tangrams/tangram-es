#include "scene/spriteAtlas.h"

namespace Tangram {

SpriteAtlas::SpriteAtlas() {}

void SpriteAtlas::addSpriteNode(const std::string& _name, glm::vec2 _origin, glm::vec2 _size) {
    m_spritesNodes[_name] = SpriteNode { {}, {}, _size, _origin};
}

bool SpriteAtlas::getSpriteNode(const std::string& _name, SpriteNode& _node) const {
    auto it = m_spritesNodes.find(_name);
    if (it == m_spritesNodes.end()) {
        return false;
    }

    _node = it->second;
    return true;
}

void SpriteAtlas::updateSpriteNodes(const glm::vec2& _textureSize) {
    float atlasWidth = _textureSize.x;
    float atlasHeight = _textureSize.y;

    for (auto& spriteNode : m_spritesNodes) {

        const auto& origin = spriteNode.second.m_origin;
        const auto& size = spriteNode.second.m_size;

        float uvL = origin.x / atlasWidth;
        float uvR = uvL + size.x / atlasWidth;
        float uvB = 1.f - origin.y / atlasHeight;
        float uvT = uvB - size.y / atlasHeight;

        spriteNode.second.m_uvBL = { uvL, uvB };
        spriteNode.second.m_uvTR = { uvR, uvT };

    }
}

}
