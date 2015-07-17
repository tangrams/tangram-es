#pragma once

#include <map>
#include <memory>
#include "gl/texture.h"
#include "glm/glm.hpp"

struct SpriteNode {
    glm::vec2 uvBL;
    glm::vec2 uvTR;
    glm::vec2 size;
};

class SpriteAtlas {

public:
    SpriteAtlas(const std::string& _file);

    void addSpriteNode(const std::string& _name, glm::vec2 _origin, glm::vec2 _size);
    SpriteNode getSpriteNode(const std::string& _name);

    void bind(GLuint _slot = 0);
    bool hasSpriteNode(const std::string& _name) const;

private:
    std::map<std::string, SpriteNode> m_spritesNodes;
    std::string m_file;
    std::shared_ptr<Texture> m_texture;
};
