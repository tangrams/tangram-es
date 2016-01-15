#include "spriteAtlas.h"
#include "platform.h"

namespace Tangram {

SpriteAtlas::SpriteAtlas(std::shared_ptr<Texture> _texture, const std::string& _file) : m_file(_file), m_texture(_texture) {}

void SpriteAtlas::addSpriteNode(const std::string& _name, glm::vec2 _origin, glm::vec2 _size) {

    glm::vec2 atlasSize = {m_texture->getWidth(), m_texture->getHeight()};
    glm::vec2 uvBL = _origin / atlasSize;
    glm::vec2 uvTR = (_origin + _size) / atlasSize;

    m_spritesNodes[_name] = SpriteNode { uvBL, uvTR, _size };
}

bool SpriteAtlas::getSpriteNode(const std::string& _name, SpriteNode& _node) const {
    auto it = m_spritesNodes.find(_name);
    if (it == m_spritesNodes.end()) {
        return false;
    }

    _node = it->second;
    return true;
}

void SpriteAtlas::bind(GLuint _slot) {
    m_texture->update(_slot);
    m_texture->bind(_slot);
}

}
