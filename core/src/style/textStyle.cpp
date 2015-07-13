#include "textStyle.h"

#include "text/fontContext.h"
#include "tile/mapTile.h"
#include "util/shaderProgram.h"
#include "util/vboMesh.h"
#include "view/view.h"
#include "textBatch.h"

TextStyle::TextStyle(const std::string& _fontName, std::string _name, float _fontSize, unsigned int _color, bool _sdf, bool _sdfMultisampling, GLenum _drawMode)
: Style(_name, _drawMode), m_fontName(_fontName), m_fontSize(_fontSize), m_color(_color), m_sdf(_sdf), m_sdfMultisampling(_sdfMultisampling)  {
    m_labels = Labels::GetInstance();
}

TextStyle::~TextStyle() {
}

void TextStyle::constructVertexLayout() {
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 2, GL_FLOAT, false, 0},
        {"a_uv", 2, GL_FLOAT, false, 0},
        {"a_screenPosition", 2, GL_FLOAT, false, 0},
        {"a_alpha", 1, GL_FLOAT, false, 0},
        {"a_rotation", 1, GL_FLOAT, false, 0},
    }));
}

void TextStyle::constructShaderProgram() {
    std::string frag = m_sdf ? "sdf.fs" : "text.fs";

    std::string vertShaderSrcStr = stringFromResource("point.vs");
    std::string fragShaderSrcStr = stringFromResource(frag.c_str());

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);

    std::string defines;

    if (m_sdf && m_sdfMultisampling) {
        defines += "#define TANGRAM_SDF_MULTISAMPLING\n";
    }

    m_shaderProgram->addSourceBlock("defines", defines);
}

void TextStyle::onBeginDrawFrame(const std::shared_ptr<View>& _view, const std::shared_ptr<Scene>& _scene) {
    auto ftContext = m_labels->getFontContext();
    const auto& atlas = ftContext->getAtlas();
    float projectionMatrix[16] = {0};

    ftContext->setScreenSize(_view->getWidth(), _view->getHeight());
    ftContext->getProjection(projectionMatrix);

    atlas->update(1);
    atlas->bind(1);

    m_shaderProgram->setUniformi("u_tex", 1);
    m_shaderProgram->setUniformf("u_resolution", _view->getWidth(), _view->getHeight());

    float r = (m_color >> 16 & 0xff) / 255.0;
    float g = (m_color >> 8  & 0xff) / 255.0;
    float b = (m_color       & 0xff) / 255.0;

    m_shaderProgram->setUniformf("u_color", r, g, b);
    m_shaderProgram->setUniformMatrix4f("u_proj", projectionMatrix);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
}

void TextStyle::onEndDrawFrame() {
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}


StyleBatch* TextStyle::newBatch() const {
    auto batch = new TextBatch(*this);
    batch->init();

    auto ftContext = m_labels->getFontContext();

    ftContext->setFont(m_fontName, m_fontSize * m_pixelScale);
    if (m_sdf) {
        float blurSpread = 2.5;
        ftContext->setSignedDistanceField(blurSpread);
    }

    return batch;
};

