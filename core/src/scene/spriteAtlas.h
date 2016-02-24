#pragma once

#include <map>
#include <memory>
#include "gl/texture.h"
#include "glm/glm.hpp"

namespace Tangram {

struct SpriteNode {
    glm::vec2 uvBL;
    glm::vec2 uvTR;
    glm::vec2 size;
    glm::vec2 origin;
};

class SpriteAtlas {

public:
    SpriteAtlas(std::shared_ptr<Texture> _texture, const std::string& _file);

    /* Creates a sprite node in the atlas located at _origin in the texture by a size in pixels _size */
    void addSpriteNode(const std::string& _name, glm::vec2 _origin, glm::vec2 _size);
    bool getSpriteNode(const std::string& _name, SpriteNode& _node) const;

    /* Bind the atlas in the driver */
    void bind(GLuint _slot);
    bool pack();

private:
    struct PackedNode {
        int x, y, width;
    };

    int rectFits(uint32_t _i, int _w, int _h);
    void addSkylineLevel(uint32_t _idx, int _x, int _y, int _w, int _h);
    bool addNode(const SpriteNode& _in, SpriteNode& _out);

    std::map<std::string, SpriteNode> m_spritesNodes;
    std::string m_file;
    std::shared_ptr<Texture> m_texture;
    
    std::vector<PackedNode> m_packedNodes;
};

}
