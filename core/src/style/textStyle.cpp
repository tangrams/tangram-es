#include "textStyle.h"

#include "text/fontContext.h"
#include "tile/mapTile.h"
#include "util/shaderProgram.h"
#include "util/vboMesh.h"
#include "view/view.h"

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

void TextBatch::add(const Feature& _feature, const StyleParamMap& _params, const MapTile& _tile) {

    switch (_feature.geometryType) {
        case GeometryType::points:
            for (auto& point : _feature.points) {
                buildPoint(point, _feature.props, _tile);
            }
            break;
        case GeometryType::lines:
            for (auto& line : _feature.lines) {
                buildLine(line, _feature.props, _tile);
            }
            break;
        case GeometryType::polygons:
            for (auto& polygon : _feature.polygons) {
                buildPolygon(polygon, _feature.props, _tile);
            }
            break;
        default:
            break;
    }
}

void TextBatch::buildPoint(const Point& _point, const Properties& _props, const MapTile& _tile) {

    std::string text;
    if (!_props.getString("name", text)) {
        return;
    }

    auto label = m_style.m_labels->addTextLabel(*this, _tile, { glm::vec2(_point), glm::vec2(_point) },
                                                text, Label::Type::point);
    if (label) {
        m_labels.push_back(label);
    }

    m_style.m_labels->addTextLabel(*this, _tile, { glm::vec2(_point), glm::vec2(_point) }, text, Label::Type::point);
}

void TextBatch::buildLine(const Line& _line, const Properties& _props, const MapTile& _tile) {

    std::string text;
    if (!_props.getString("name", text)) {
        return;
    }

    int lineLength = _line.size();
    int skipOffset = floor(lineLength / 2);
    float minLength = 0.15; // default, probably need some more thoughts


    for (size_t i = 0; i < _line.size() - 1; i += skipOffset) {
        glm::vec2 p1 = glm::vec2(_line[i]);
        glm::vec2 p2 = glm::vec2(_line[i + 1]);

        glm::vec2 p1p2 = p2 - p1;
        float length = glm::length(p1p2);

        if (length < minLength) {
            continue;
        }

       auto label =  m_style.m_labels->addTextLabel(*this, _tile, { p1, p2 }, text, Label::Type::line);
        if (label) {
            m_labels.push_back(label);
        }
    }
}

void TextBatch::buildPolygon(const Polygon& _polygon, const Properties& _props, const MapTile& _tile) {

    std::string text;
    if (!_props.getString("name", text)) {
        return;
    }

    glm::vec3 centroid;
    int n = 0;

    for (auto& l : _polygon) {
        for (auto& p : l) {
            centroid.x += p.x;
            centroid.y += p.y;
            n++;
        }
    }

    centroid /= n;

    auto label = m_style.m_labels->addTextLabel(*this, _tile,
                                                { glm::vec2(centroid), glm::vec2(centroid) },
                                                text, Label::Type::point);
    if (label) {
        m_labels.push_back(label);
    }

}

void TextBatch::onBeginBuildTile() {
    //init();
}

void TextBatch::onEndBuildTile() {
    addBufferVerticesToMesh();
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

    RenderState::blending(GL_TRUE);
    RenderState::blendingFunc({GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA});
    RenderState::depthTest(GL_FALSE);
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
