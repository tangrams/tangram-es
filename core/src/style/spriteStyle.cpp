#include "spriteStyle.h"
#include "spriteBatch.h"

#include "tile/labels/labels.h"
#include "tile/labels/spriteAtlas.h"

#include "platform.h"
#include "util/shaderProgram.h"
#include "util/texture.h"
#include "util/vertexLayout.h"
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

void SpriteStyle::onBeginDrawFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) {
    m_spriteAtlas->bind();

    m_shaderProgram->setUniformi("u_tex", 0);
    // top-left screen axis, y pointing down
    m_shaderProgram->setUniformMatrix4f("u_proj", glm::value_ptr(glm::ortho(0.f, _view->getWidth(),
                                                                            _view->getHeight(),
                                                                            0.f, -1.f, 1.f)));

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void SpriteStyle::onEndDrawFrame() {
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

StyleBatch* SpriteStyle::newBatch() const {
    return new SpriteBatch(*this);
}
