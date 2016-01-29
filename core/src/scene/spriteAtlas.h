#pragma once

#include <map>
#include <memory>
#include "gl/texture.h"
#include "glm/glm.hpp"

namespace Tangram {

struct SpriteNode {
    glm::vec2 m_uvBL;
    glm::vec2 m_uvTR;
    glm::vec2 m_size;
};

class SpriteAtlas {

public:
    SpriteAtlas(std::shared_ptr<Texture> _texture, const std::string& _file);

    /* Creates a sprite node in the atlas located at _origin in the texture by a size in pixels _size */
    void addSpriteNode(const std::string& _name, glm::vec2 _origin, glm::vec2 _size);
    bool getSpriteNode(const std::string& _name, SpriteNode& _node) const;

    /* Bind the atlas in the driver */
    void bind(GLuint _slot);

private:
    std::map<std::string, SpriteNode> m_spritesNodes;
    std::string m_file;
    std::shared_ptr<Texture> m_texture;
};

}
