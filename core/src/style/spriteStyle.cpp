#include "spriteStyle.h"

#include "platform.h"
#include "tile/mapTile.h"
#include "util/shaderProgram.h"
#include "util/texture.h"
#include "util/vertexLayout.h"
#include "util/builders.h"
#include "view/view.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

SpriteStyle::SpriteStyle(std::string _name, GLenum _drawMode) : Style(_name, _drawMode) {
    
    m_labels = Labels::GetInstance();
}

SpriteStyle::~SpriteStyle() {}

void SpriteStyle::constructVertexLayout() {
    
    // 32 bytes, good for memory aligments
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_FLOAT, false, 0},
        {"a_uv", 2, GL_FLOAT, false, 0},
        {"a_screenPosition", 2, GL_FLOAT, false, 0},
        {"a_alpha", 1, GL_FLOAT, false, 0},
        {"a_rotation", 1, GL_FLOAT, false, 0},
    }));
}

void SpriteStyle::constructShaderProgram() {

    std::string fragShaderSrcStr = stringFromResource("sprite.fs");
    std::string vertShaderSrcStr = stringFromResource("point.vs");

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);
    
    // TODO : load this from stylesheet
    m_spriteAtlas = std::unique_ptr<SpriteAtlas>(new SpriteAtlas("poi_icons_32.png"));
    
    m_spriteAtlas->addSpriteNode("plane", {0, 0}, {32, 32});
    m_spriteAtlas->addSpriteNode("tree", {0, 185}, {32, 32});
    m_spriteAtlas->addSpriteNode("sunburst", {0, 629}, {32, 32});
}

void SpriteStyle::buildPoint(Point& _point, const StyleParamMap&, Properties& _props, VboMesh& _mesh, MapTile& _tile) const {
    // TODO : make this configurable
    SpriteNode planeSprite = m_spriteAtlas->getSpriteNode("tree");
    std::vector<BufferVert> vertices;
    
    SpriteBuilder builder = {
        [&](const glm::vec2& coord, const glm::vec2& screenPos, const glm::vec2& uv) {
            vertices.push_back({ coord, uv, screenPos, 0.5f, M_PI_2 });
        }
    };
    
    // TODO : configure this
    float spriteScale = .5f;
    glm::vec2 offset = {0.f, 10.f};
    
    for (auto prop : _props.stringProps) {
        if (prop.first == "name") {
            Label::Transform t = {glm::vec2(_point), glm::vec2(_point)};
            
            SpriteLabel::AttributeOffsets attribOffsets = {
                _mesh.numVertices() * m_vertexLayout->getStride(),
                (GLintptr) m_vertexLayout->getOffset("a_screenPosition"),
                (GLintptr) m_vertexLayout->getOffset("a_rotation"),
                (GLintptr) m_vertexLayout->getOffset("a_alpha"),
            };
            
            auto label = m_labels->addSpriteLabel(_tile, m_name, t, planeSprite.size * spriteScale, offset, attribOffsets);
            
            if (label) {
                Builders::buildQuadAtPoint(label->getTransform().m_screenPosition + offset, planeSprite.size * spriteScale, planeSprite.uvBL, planeSprite.uvTR, builder);
            }
        }
    }
    
    auto& mesh = static_cast<SpriteStyle::Mesh&>(_mesh);
    mesh.addVertices(std::move(vertices), std::move(builder.indices));
}

void SpriteStyle::buildLine(Line& _line, const StyleParamMap&, Properties& _props, VboMesh& _mesh, MapTile& _tile) const {
    // NO-OP
}

void SpriteStyle::buildPolygon(Polygon& _polygon, const StyleParamMap&, Properties& _props, VboMesh& _mesh, MapTile& _tile) const {
    // NO-OP
}

void SpriteStyle::onBeginDrawFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) {
    m_spriteAtlas->bind();
    
    m_shaderProgram->setUniformi("u_tex", 0);
    // top-left screen axis, y pointing down
    m_shaderProgram->setUniformMatrix4f("u_proj", glm::value_ptr(glm::ortho(0.f, _view->getWidth(), _view->getHeight(), 0.f, -1.f, 1.f)));
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void SpriteStyle::onEndDrawFrame() {
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}
