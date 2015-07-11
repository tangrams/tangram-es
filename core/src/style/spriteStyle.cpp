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

SpriteBatch::SpriteBatch(const SpriteStyle& _style)
    :  m_mesh(std::make_shared<TypedMesh<BufferVert>>(_style.m_vertexLayout, GL_TRIANGLES, GL_DYNAMIC_DRAW)),
       m_style(_style)
{
        
    // m_dirtyTransform = false;
    // m_bound = false;
}

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

    // NB: byte offset into BufferVert 'state'
    m_stateAttribOffset = m_vertexLayout->getOffset("a_screenPosition");
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

void SpriteStyle::buildPoint(Point& _point, const StyleParamMap& _styleParamMap, Properties& _props, Batch& _batch, MapTile& _tile) const {
    auto& batch = static_cast<SpriteBatch&>(_batch);

    if (!_props.contains("name"))
        return;

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

    Label::Transform t = {glm::vec2(_point), glm::vec2(_point)};

    size_t bufferOffset = batch.m_mesh->numVertices() * m_vertexLayout->getStride() + m_stateAttribOffset;
            
    auto label = m_labels->addSpriteLabel(batch, _tile, t,
                                          planeSprite.size * spriteScale,
                                          offset, bufferOffset);
    if (label) {
        Builders::buildQuadAtPoint(label->getTransform().state.screenPos + offset,
                                   planeSprite.size * spriteScale,
                                   planeSprite.uvBL, planeSprite.uvTR, builder);
    }
    
    batch.m_mesh->addVertices(std::move(vertices), std::move(builder.indices));
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

Batch* SpriteStyle::newBatch() const {
    return new SpriteBatch(*this);
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
        label->pushTransform(*this);
    }
}
