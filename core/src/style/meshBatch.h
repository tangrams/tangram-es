#pragma once

#include "style/styleBatch.h"
#include "view/view.h"

#include <memory>

template<class S, class M>
class MeshBatch : public StyleBatch {

public:

    MeshBatch(const S& _style, std::shared_ptr<M> _mesh) : m_style(_style), m_mesh(_mesh) {}

    virtual void draw(const View& _view) override {
            m_mesh->draw(m_style.getShaderProgram());
    }

    virtual void update(const glm::mat4& mvp, const View& _view, float _dt) override {}

    virtual void prepare() override {}

    virtual bool compile() override {
        if (m_mesh->numVertices() > 0) {
            m_mesh->compileVertexBuffer();
            return true;
        }
        return false;
    }

    virtual void add(const Feature& _feature, const StyleParamMap& _params, const MapTile& _tile) override {}

protected:
    const S& m_style;
    std::shared_ptr<M> m_mesh;
};
