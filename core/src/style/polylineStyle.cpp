#include "polylineStyle.h"
#include "util/builders.h"
#include "roadLayers.h"
#include "tangram.h"
#include <ctime>

PolylineStyle::PolylineStyle(std::string _name, GLenum _drawMode) : Style(_name, _drawMode) {    
    constructVertexLayout();
    constructShaderProgram();
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
    
    m_shaderProgram = std::make_shared<ShaderProgram>();
    m_shaderProgram->setSourceStrings(fragShaderSrcStr, vertShaderSrcStr);
}

void PolylineStyle::buildPoint(Point& _point, StyleParams& _params, Properties& _props, VboMesh& _mesh) const {
    // No-op
}

void PolylineStyle::buildLine(Line& _line, StyleParams& _params, Properties& _props, VboMesh& _mesh) const {
    std::vector<PosNormEnormColVertex> vertices;
    std::vector<int> indices;
    std::vector<glm::vec3> points;
    std::vector<glm::vec2> texcoords;
    std::vector<glm::vec2> scalingVecs;
    
    GLuint abgr = _params.color;
    
    if (Tangram::getDebugFlag(Tangram::DebugFlags::PROXY_COLORS)) {
        abgr = abgr << (int(_props.numericProps["zoom"]) % 6);
    }
    
    GLfloat layer = _props.numericProps["sort_key"] + _params.order;
    
    float halfWidth = _params.width * .5f;
    
    PolyLineOutput lineOutput = { points, indices, scalingVecs, texcoords };
    PolyLineOptions lineOptions = { _params.line.cap, _params.line.join, halfWidth };
    Builders::buildPolyLine(_line, lineOptions, lineOutput);
    
    // populate polyline vertices
    for (size_t i = 0; i < points.size(); i++) {
        const glm::vec3& p = points[i];
        const glm::vec2& uv = texcoords[i];
        const glm::vec2& en = scalingVecs[i];
        vertices.push_back({ p, uv, en, halfWidth, abgr, layer });
    }
    
    if (_params.outline.on) {

        GLuint abgrOutline = _params.outline.color;
        halfWidth += _params.outline.width * .5f;
        
        size_t outlineStart = 0;
        
        if (_params.outline.line.cap != _params.line.cap || _params.outline.line.join != _params.line.join) {
            // need to re-triangulate with different cap and/or join
            outlineStart = points.size();
            lineOptions.cap = _params.outline.line.cap;
            lineOptions.join = _params.outline.line.join;
            Builders::buildPolyLine(_line, lineOptions, lineOutput);

        } else {

            // re-use indices from original line
            size_t oldSize = indices.size();
            size_t offset = points.size();
            indices.reserve(2 * oldSize);
            for(size_t i = 0; i < oldSize; i++) {
                indices.push_back(offset + indices[i]);
            }

        }

        // populate outline vertices
        for (size_t i = outlineStart; i < points.size(); i++) {
            const glm::vec3& p = points[i];
            const glm::vec2& uv = texcoords[i];
            const glm::vec2& en = scalingVecs[i];
            vertices.push_back({ p, uv, en, halfWidth, abgrOutline, layer - 1.f });
        }
        
    }

    auto& mesh = static_cast<PolylineStyle::Mesh&>(_mesh);
    mesh.addVertices(std::move(vertices), std::move(indices));
}

void PolylineStyle::buildPolygon(Polygon& _polygon, StyleParams& _params, Properties& _props, VboMesh& _mesh) const {
    // No-op
}
