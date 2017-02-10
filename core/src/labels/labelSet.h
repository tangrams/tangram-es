#pragma once

#include "labels/label.h"
#include "style/style.h"

#include <vector>
#include <memory>

namespace Tangram {

class LabelSet : public StyledMesh {
public:

    const auto& getLabels() const { return m_labels; }
    auto& getLabels() { return m_labels; }

    virtual ~LabelSet();

    bool draw(RenderState& rs, ShaderProgram& _shader, bool _useVao = true) override { return true; }

    size_t bufferSize() const override { return 0; }

    void setLabels(std::vector<std::unique_ptr<Label>>& _labels);

    void reset();

protected:
    std::vector<std::unique_ptr<Label>> m_labels;
};

}
