#include "spriteBatch.h"

#include "spriteStyle.h"

#include "tile/labels/labels.h"
#include "tile/labels/spriteAtlas.h"

#include "util/builders.h"
#include "view/view.h"

SpriteBatch::SpriteBatch(const SpriteStyle& _style)
    :  m_mesh(std::make_shared<TypedMesh<BufferVert>>(_style.m_vertexLayout, GL_TRIANGLES, GL_DYNAMIC_DRAW)),
       m_style(_style)
{
    // m_dirtyTransform = false;
    // m_bound = false;
}

void SpriteBatch::add(const Feature& _feature, const StyleParamMap& _params, const MapTile& _tile) {

    if (_feature.geometryType != GeometryType::points) {
        return;
    }

    for (auto& point : _feature.points) {
        buildPoint(point, _feature.props, _tile);
    }
}

void SpriteBatch::buildPoint(const Point& _point, const Properties& _props, const MapTile& _tile) {

    // TODO : make this configurable
    SpriteNode planeSprite = m_style.m_spriteAtlas->getSpriteNode("tree");
    std::vector<BufferVert> vertices;

    SpriteBuilder builder = {
        [&](const glm::vec2& coord, const glm::vec2& screenPos, const glm::vec2& uv) {
            vertices.push_back({ coord, uv, screenPos, 0.5f, M_PI_2 });
        }
    };

    // TODO : configure this
    float spriteScale = .5f;
    glm::vec2 offset = {0.f, 10.f};

    // NB: byte offset into BufferVert 'state'
    size_t attribOffset =  (size_t)m_style.m_vertexLayout->getOffset("a_screenPosition");

    for (auto prop : _props.stringProps) {
        if (prop.first == "name") {
            Label::Transform t = {glm::vec2(_point), glm::vec2(_point)};

            size_t bufferOffset = m_mesh->numVertices() * m_style.m_vertexLayout->getStride() + attribOffset;

            std::shared_ptr<SpriteLabel> label = m_style.m_labels->addSpriteLabel(*this, _tile, t,
                                                                                  planeSprite.size * spriteScale,
                                                                                  offset, bufferOffset);

            if (label) {
                m_labels.push_back(label);

                Builders::buildQuadAtPoint(label->getTransform().state.screenPos + offset,
                                           planeSprite.size * spriteScale,
                                           planeSprite.uvBL, planeSprite.uvTR, builder);
            }
        }
    }

    m_mesh->addVertices(std::move(vertices), std::move(builder.indices));
}

void SpriteBatch::draw(const View& _view) {
    m_mesh->draw(m_style.getShaderProgram());
}

void SpriteBatch::update(const glm::mat4& mvp, const View& _view, float _dt) {
    glm::vec2 screenSize = glm::vec2(_view.getWidth(), _view.getHeight());
    for (auto& label : m_labels) {
        label->update(mvp, screenSize, _dt);
    }
}

void SpriteBatch::prepare() {
    for(auto& label : m_labels) {
        label->pushTransform(*m_mesh);
    }
}

bool SpriteBatch::compile() {
    if (m_mesh->numVertices() > 0) {
        m_mesh->compileVertexBuffer();
        return true;
    }
    return false;
};
