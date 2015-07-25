#include "spriteAtlas.h"
#include "platform.h"

namespace Tangram {

SpriteAtlas::SpriteAtlas(const std::string& _file) : m_file(_file) {
    m_texture = std::shared_ptr<Texture>(new Texture(_file));
}

void SpriteAtlas::addSpriteNode(const std::string& _name, glm::vec2 _origin, glm::vec2 _size) {

    glm::vec2 atlasSize = {m_texture->getWidth(), m_texture->getHeight()};
    glm::vec2 uvBL = _origin / atlasSize;
    glm::vec2 uvTR = (_origin + _size) / atlasSize;

    m_spritesNodes[_name] = SpriteNode { uvBL, uvTR, _size };
}

SpriteNode SpriteAtlas::getSpriteNode(const std::string& _name) {
    if (!hasSpriteNode(_name)) {
        logMsg("WARNING: trying to access unrecognized sprite node named %s\n", _name.c_str());
        return {};
    }

    return m_spritesNodes.at(_name);
}

bool SpriteAtlas::hasSpriteNode(const std::string& _name) const {
    return m_spritesNodes.find(_name) != m_spritesNodes.end();
}

void SpriteAtlas::bind(GLuint _slot) {
    m_texture->update(_slot);
    m_texture->bind(_slot);
}

}
