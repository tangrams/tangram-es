#pragma once

#include <vector>
#include <memory>
#include "label.h"
#include "style/style.h"

namespace Tangram {

class LabelSet : public StyledMesh {
public:

    const auto& getLabels() const { return m_labels; }
    auto& getLabels() { return m_labels; }

    virtual ~LabelSet();

    bool draw(ShaderProgram& _shader) override { return true; }

    size_t bufferSize() const override { return 0; }

    void setLabels(std::vector<std::unique_ptr<Label>>& _labels);

    void reset();

protected:
    std::vector<std::unique_ptr<Label>> m_labels;
};

}
