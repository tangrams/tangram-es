#include "scene/spriteAtlas.h"

#include "platform.h"

namespace Tangram {

SpriteAtlas::SpriteAtlas(std::shared_ptr<Texture> _texture) : m_texture(_texture) {}

void SpriteAtlas::addSpriteNode(const std::string& _name, glm::vec2 _origin, glm::vec2 _size) {

    float atlasWidth = m_texture->getWidth();
    float atlasHeight = m_texture->getHeight();
    float uvL = _origin.x / atlasWidth;
    float uvR = uvL + _size.x / atlasWidth;
    float uvB = 1.f - _origin.y / atlasHeight;
    float uvT = uvB - _size.y / atlasHeight;

    m_spritesNodes[_name] = SpriteNode { { uvL, uvB }, { uvR, uvT }, _size, _origin };
}

bool SpriteAtlas::getSpriteNode(const std::string& _name, SpriteNode& _node) const {
    auto it = m_spritesNodes.find(_name);
    if (it == m_spritesNodes.end()) {
        return false;
    }

    _node = it->second;
    return true;
}

void SpriteAtlas::updateSpriteNodes(std::shared_ptr<Texture> _texture) {
    m_texture = _texture;
    for (auto& spriteNode : m_spritesNodes) {
        // Use the origin of the spriteNode set when the node was created
        addSpriteNode(spriteNode.first.k, spriteNode.second.m_origin, spriteNode.second.m_size);
    }
}

void SpriteAtlas::bind(RenderState& rs, GLuint _slot) {
    m_texture->update(rs, _slot);
    m_texture->bind(rs, _slot);
}

}
