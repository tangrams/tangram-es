#include "polygonStyle.h"

#include "polygonBatch.h"
#include "util/builders.h"
#include "util/shaderProgram.h"

PolygonStyle::PolygonStyle(std::string _name, GLenum _drawMode) : Style(_name, _drawMode) {
}

void PolygonStyle::constructVertexLayout() {

    // TODO: Ideally this would be in the same location as the struct that it basically describes
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 3, GL_FLOAT, false, 0},
        {"a_normal", 3, GL_FLOAT, false, 0},
        {"a_texcoord", 2, GL_FLOAT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_layer", 1, GL_FLOAT, false, 0}
    }));

}

void PolygonStyle::constructShaderProgram() {

    std::string vertShaderSrcStr = stringFromResource("polygon.vs");
    std::string fragShaderSrcStr = stringFromResource("polygon.fs");

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);
}

StyleBatch* PolygonStyle::newBatch() const {
    return new PolygonBatch(*this);
};


auto PolygonStyle::parseParamMap(const StyleParamMap& _styleParams) const -> StyleParams {
    StyleParams params;

    if (_styleParams.find("order") != _styleParams.end()) {
        params.order = std::stof(_styleParams.at("order"));
    }
    if (_styleParams.find("color") != _styleParams.end()) {
        params.color = Style::parseColorProp(_styleParams.at("color"));
    }

    return params;
}
