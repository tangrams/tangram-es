#include "polylineStyle.h"

#include "polylineBatch.h"
#include "util/shaderProgram.h"

PolylineStyle::PolylineStyle(std::string _name, GLenum _drawMode) : Style(_name, _drawMode) {
}

void PolylineStyle::constructVertexLayout() {

    // TODO: Ideally this would be in the same location as the struct that it basically describes
    m_vertexLayout = std::shared_ptr<VertexLayout>(new VertexLayout({
        {"a_position", 3, GL_FLOAT, false, 0},
        {"a_texcoord", 2, GL_FLOAT, false, 0},
        {"a_extrudeNormal", 2, GL_FLOAT, false, 0},
        {"a_extrudeWidth", 1, GL_FLOAT, false, 0},
        {"a_color", 4, GL_UNSIGNED_BYTE, true, 0},
        {"a_layer", 1, GL_FLOAT, false, 0}
    }));

}

void PolylineStyle::constructShaderProgram() {

    std::string vertShaderSrcStr = stringFromResource("polyline.vs");
    std::string fragShaderSrcStr = stringFromResource("polyline.fs");

    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);
}

StyleBatch* PolylineStyle::newBatch() const {
    return new PolylineBatch(*this);
};

auto PolylineStyle::parseParamMap(const StyleParamMap& _styleParams) const -> StyleParams {

    StyleParams params;

    if(_styleParams.find("order") != _styleParams.end()) {
        params.order = std::stof(_styleParams.at("order"));
    }

    if(_styleParams.find("color") != _styleParams.end()) {
        params.color = Style::parseColorProp(_styleParams.at("color"));
    }

    if(_styleParams.find("width") != _styleParams.end()) {
        params.width = std::stof(_styleParams.at("width"));
    }

    if(_styleParams.find("cap") != _styleParams.end()) {
        std::string capStr = _styleParams.at("cap");
        if(capStr == "butt") { params.cap = CapTypes::butt; }
        else if(capStr == "square") { params.cap = CapTypes::square; }
        else if(capStr == "round") { params.cap = CapTypes::round; }
    }

    if(_styleParams.find("join") != _styleParams.end()) {
        std::string joinStr = _styleParams.at("join");
        if(joinStr == "bevel") { params.join = JoinTypes::bevel; }
        else if(joinStr == "miter") { params.join = JoinTypes::miter; }
        else if(joinStr == "round") { params.join = JoinTypes::round; }
    }

    if(_styleParams.find("outline:width") != _styleParams.end()) {
        params.outlineOn = true;
        params.outlineWidth = std::stof(_styleParams.at("outline:width"));
    }

    if(_styleParams.find("outline:color") != _styleParams.end()) {
        params.outlineColor =  Style::parseColorProp(_styleParams.at("outline:color"));
    }

    if(_styleParams.find("outline:cap") != _styleParams.end()) {
        params.outlineOn = true;
        std::string capStr = _styleParams.at("outline:cap");
        if(capStr == "butt") { params.outlineCap = CapTypes::butt; }
        else if(capStr == "square") { params.outlineCap = CapTypes::square; }
        else if(capStr == "round") { params.outlineCap = CapTypes::round; }
    }

    if( _styleParams.find("outline:join") != _styleParams.end()) {
        params.outlineOn = true;
        std::string joinStr = _styleParams.at("outline:join");
        if(joinStr == "bevel") { params.outlineJoin = JoinTypes::bevel; }
        else if(joinStr == "miter") { params.outlineJoin = JoinTypes::miter; }
        else if(joinStr == "round") { params.outlineJoin = JoinTypes::round; }
    }

    return params;
}
