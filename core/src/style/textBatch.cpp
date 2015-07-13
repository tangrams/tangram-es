#include "textBatch.h"

#include "textStyle.h"
#include "text/textBuffer.h"
#include "text/fontContext.h"
#include "view/view.h"

TextBatch::TextBatch(const TextStyle& _style)
    : m_fontContext(_style.m_labels->getFontContext()),
      m_mesh(std::make_shared<TypedMesh<BufferVert>>(_style.m_vertexLayout, GL_TRIANGLES, GL_DYNAMIC_DRAW)),
      m_style(_style)
{

    m_dirtyTransform = false;
    m_bound = false;
}

void TextBatch::init() {
    m_fontContext->lock();
    glfonsBufferCreate(m_fontContext->getFontContext(), &m_fsBuffer);
    m_fontContext->unlock();
}

TextBatch::~TextBatch() {
    glfonsBufferDelete(m_fontContext->getFontContext(), m_fsBuffer);
}

int TextBatch::getVerticesSize() {
    bind();
    int size = glfonsVerticesSize(m_fontContext->getFontContext());
    unbind();
    return size;
}

fsuint TextBatch::genTextID() {
    fsuint id;
    bind();
    glfonsGenText(m_fontContext->getFontContext(), 1, &id);
    unbind();
    return id;
}

bool TextBatch::rasterize(const std::string& _text, fsuint _id) {
    bind();
    int status = glfonsRasterize(m_fontContext->getFontContext(), _id, _text.c_str());
    unbind();
    return status == GLFONS_VALID;
}

void TextBatch::pushBuffer() {
    if (m_dirtyTransform) {
        bind();
        glfonsUpdateBuffer(m_fontContext->getFontContext(), m_mesh.get());
        unbind();
        m_dirtyTransform = false;
    }
}

void TextBatch::transformID(fsuint _textID, const BufferVert::State& _state) {
    bind();
    glfonsTransform(m_fontContext->getFontContext(), _textID,
                    _state.screenPos.x, _state.screenPos.y,
                    _state.rotation, _state.alpha);
    unbind();
    m_dirtyTransform = true;
}

glm::vec4 TextBatch::getBBox(fsuint _textID) {
    glm::vec4 bbox;
    bind();
    glfonsGetBBox(m_fontContext->getFontContext(), _textID, &bbox.x, &bbox.y, &bbox.z, &bbox.w);
    unbind();
    return bbox;
}

void TextBatch::addBufferVerticesToMesh() {
    std::vector<BufferVert> vertices;
    int bufferSize = getVerticesSize();

    if (bufferSize == 0) {
        return;
    }

    vertices.resize(bufferSize);

    bind();
    bool res = glfonsVertices(m_fontContext->getFontContext(), reinterpret_cast<float*>(vertices.data()));
    unbind();

    if (res) {
        m_mesh->addVertices(std::move(vertices), {});
    }
}

void TextBatch::bind() {
    if (!m_bound) {
        m_fontContext->lock();
        glfonsBindBuffer(m_fontContext->getFontContext(), m_fsBuffer);
        m_bound = true;
    }
}

void TextBatch::unbind() {
    if (m_bound) {
        glfonsBindBuffer(m_fontContext->getFontContext(), 0);
        m_fontContext->unlock();
        m_bound = false;
    }
}

void TextBatch::draw(const View& _view) {
    m_mesh->draw(m_style.getShaderProgram());
};


void TextBatch::update(const glm::mat4& mvp, const View& _view, float _dt) {
    glm::vec2 screenSize = glm::vec2(_view.getWidth(), _view.getHeight());
    for (auto& label : m_labels) {
        label->update(mvp, screenSize, _dt);
    }
}

void TextBatch::prepare() {
    for(auto& label : m_labels) {
        label->pushTransform(*this);
    }
    pushBuffer();
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
