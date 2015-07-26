#include "labels/labelMesh.h"
#include "labels/label.h"

namespace Tangram {

LabelMesh::LabelMesh(std::shared_ptr<VertexLayout> _vertexLayout, GLenum _drawMode)
    : TypedMesh<Label::Vertex>(_vertexLayout, _drawMode, GL_DYNAMIC_DRAW) {
}

LabelMesh::~LabelMesh() {}

void LabelMesh::addLabel(std::unique_ptr<Label> _label) {
    m_labels.push_back(std::move(_label));
}

}
