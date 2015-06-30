#include "spriteAtlas.h"
#include "platform.h"

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
    if (m_spritesNodes.find(_name) == m_spritesNodes.end()) {
        logMsg("WARNING: trying to access unrecognized sprite node named %s\n", _name.c_str());
        return {};
    }
    
    return m_spritesNodes.at(_name);
}

void SpriteAtlas::bind() {
    m_texture->bind(0);
}
