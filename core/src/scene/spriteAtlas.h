#pragma once

#include "gl/texture.h"
#include "util/fastmap.h"

#include "glm/glm.hpp"
#include <map>
#include <memory>

namespace Tangram {

struct SpriteNode {
    glm::vec2 m_uvBL;
    glm::vec2 m_uvTR;
    glm::vec2 m_size;
    glm::vec2 m_origin;
};

class SpriteAtlas {

public:
    SpriteAtlas(std::shared_ptr<Texture> _texture);

    /* Creates a sprite node in the atlas located at _origin in the texture by a size in pixels _size */
    void addSpriteNode(const std::string& _name, glm::vec2 _origin, glm::vec2 _size);
    bool getSpriteNode(const std::string& _name, SpriteNode& _node) const;
    void updateSpriteNodes(std::shared_ptr<Texture> _texture);

    /* Bind the atlas in the driver */
    void bind(RenderState& rs, GLuint _slot);

    Texture* texture() { return m_texture.get(); }

private:
    fastmap<std::string, SpriteNode> m_spritesNodes;
    std::shared_ptr<Texture> m_texture;
};

}
