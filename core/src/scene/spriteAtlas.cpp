#include "spriteAtlas.h"
#include "platform.h"
#include "debug/textDisplay.h"
#include <algorithm>

namespace Tangram {

// TODO: make it auto resizable
#define SPRITE_ATLAS_PACK_SIZE 1024

SpriteAtlas::SpriteAtlas(std::shared_ptr<Texture> _texture, const std::string& _file) :
    m_file(_file),
    m_texture(_texture)
{}

void SpriteAtlas::addSpriteNode(const std::string& _name, glm::vec2 _origin, glm::vec2 _size) {
    glm::vec2 atlasSize = {m_texture->getWidth(), m_texture->getHeight()};
    glm::vec2 uvBL = _origin / atlasSize;
    glm::vec2 uvTR = (_origin + _size) / atlasSize;

    m_spritesNodes[_name] = SpriteNode { uvBL, uvTR, _size, _origin };
}

bool SpriteAtlas::getSpriteNode(const std::string& _name, SpriteNode& _node) const {
    auto it = m_spritesNodes.find(_name);
    if (it == m_spritesNodes.end()) {
        return false;
    }

    _node = it->second;
    return true;
}

// Adapted from Fontstash/Alfons, originally from Jukka Jylänki binpack algorithm
void SpriteAtlas::addSkylineLevel(uint32_t _idx, int _x, int _y, int _w, int _h) {
    m_packedNodes.insert(m_packedNodes.begin() + _idx, {_x, _y + _h, _w});

    // Delete skyline segments that fall under the shadow of the new segment.
    for (size_t i = _idx + 1; i < m_packedNodes.size(); i++) {
        int shrink = (m_packedNodes[i - 1].x + m_packedNodes[i - 1].width) - m_packedNodes[i].x;

        if (shrink > 0) {
            int nw = m_packedNodes[i].width - shrink;

            if (nw > 0) {
                m_packedNodes[i].x += shrink;
                m_packedNodes[i].width = nw;

                break;
            } else {
                m_packedNodes.erase(m_packedNodes.begin() + i);
                i--;
            }
        } else {
            break;
        }
    }

    // Merge same height skyline segments that are next to each other.
    for (size_t i = 0; i < m_packedNodes.size() - 1; i++) {
        if (m_packedNodes[i].y == m_packedNodes[i + 1].y) {
            m_packedNodes[i].width += m_packedNodes[i + 1].width;
            m_packedNodes.erase(m_packedNodes.begin() + i + 1);
            i--;
        }
    }
}

int SpriteAtlas::rectFits(uint32_t _i, int _w, int _h) {
    static const int defaultTextureWidth = SPRITE_ATLAS_PACK_SIZE;
    static const int defaultTextureHeight = SPRITE_ATLAS_PACK_SIZE;

    if (m_packedNodes[_i].x + _w > defaultTextureWidth) {
        return -1;
    }

    int spaceLeft = _w;
    int y = m_packedNodes[_i].y;

    while (spaceLeft > 0) {
        if (_i == m_packedNodes.size()) {
            return -1;
        }

        y = std::max(y, m_packedNodes[_i].y);
        if (y + _h > defaultTextureHeight) { return -1; }

        spaceLeft -= m_packedNodes[_i].width;

        ++_i;
    }

    return y;
}

bool SpriteAtlas::addNode(const SpriteNode& _in, SpriteNode& _out) {
    int besth = SPRITE_ATLAS_PACK_SIZE;
    int bestw = SPRITE_ATLAS_PACK_SIZE;
    int bestx = -1, besty = -1, besti = -1;

    for (size_t i = 0; i < m_packedNodes.size(); i++) {
        int y = rectFits(i, _in.size.x, _in.size.y);

        if (y != -1) {
            if ((y + _in.size.y < besth) ||
                ((y + _in.size.y == besth) && (m_packedNodes[i].width < bestw))) {
                besti = i;
                bestw = m_packedNodes[i].width;
                besth = y + _in.size.y;
                bestx = m_packedNodes[i].x;
                besty = y;
            }
        }
    }

    if (besti == -1) {
        return false;
    }

    // Perform the actual packing.
    addSkylineLevel(besti, bestx, besty, _in.size.x, _in.size.y);

    _out = _in;

    _out.origin.x = bestx;
    _out.origin.y = besty;

    return true;
}

bool SpriteAtlas::pack() {
    std::vector<std::pair<SpriteNode, SpriteNode>> packedNodes;

    m_packedNodes.clear();
    m_packedNodes.push_back({0, 0, SPRITE_ATLAS_PACK_SIZE});

    packedNodes.reserve(m_spritesNodes.size());

    for (auto& sprite : m_spritesNodes) {
        SpriteNode packed;

        if (!addNode(sprite.second, packed)) {
            // TODO: handle atlas resize in this case
            return false;
        }

        // update new uvs
        packed.uvBL = packed.origin / glm::vec2(SPRITE_ATLAS_PACK_SIZE);
        packed.uvTR = (packed.origin + packed.size) / glm::vec2(SPRITE_ATLAS_PACK_SIZE);

        packedNodes.emplace_back(sprite.second, packed);
        sprite.second = packed;
    }

    auto& textureData = m_texture->data();
    TextureOptions options = {GL_RGBA, GL_RGBA, {GL_LINEAR, GL_LINEAR}, {GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE} };
    std::shared_ptr<Texture> newTexture = std::make_shared<Texture>(SPRITE_ATLAS_PACK_SIZE, SPRITE_ATLAS_PACK_SIZE, options);

    GLuint* newTextureData = new GLuint[SPRITE_ATLAS_PACK_SIZE * SPRITE_ATLAS_PACK_SIZE];

    for (const auto& packedNodePair : packedNodes) {
        const auto& src = packedNodePair.first;
        const auto& dst = packedNodePair.second;

        assert(dst.size.x * dst.size.y == src.size.x * src.size.y);

        for (int y = 0; y < dst.size.y; ++y) {
            int srcy = (src.origin.y + y) * m_texture->getWidth();
            int dsty = (dst.origin.y + y) * SPRITE_ATLAS_PACK_SIZE;

            for (int x = 0; x < dst.size.x; ++x) {
                int srcx = src.origin.x + x;
                int dstx = dst.origin.x + x;

                newTextureData[dstx + dsty] = textureData[srcx + srcy];
            }
        }
    }

    size_t origSize = (m_texture->getWidth() * m_texture->getHeight() * sizeof(GLuint)) / (1024 * 1024);
    size_t newSize = (SPRITE_ATLAS_PACK_SIZE * SPRITE_ATLAS_PACK_SIZE * sizeof(GLuint)) / (1024 * 1024);
    size_t diff = origSize - newSize;
    LOGS("Original uncompressed size %d mb", origSize);
    LOGS("New uncompressed size %d mb", newSize);
    LOGS("Saved memory %d mb", diff);
    newTexture->setData(newTextureData, SPRITE_ATLAS_PACK_SIZE * SPRITE_ATLAS_PACK_SIZE);
    delete[] newTextureData;

    // FIXME: don't copy texture like this, keep a blob of data from original texture
    m_texture = newTexture;

    return true;
}

void SpriteAtlas::bind(GLuint _slot) {
    m_texture->update(_slot);
    m_texture->bind(_slot);
}

}
