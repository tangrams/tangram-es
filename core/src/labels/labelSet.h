#pragma once

#include <vector>
#include <memory>
#include "label.h"
#include "style/style.h"

namespace Tangram {

class LabelSet : public StyledMesh {
public:
    const std::vector<std::unique_ptr<Label>>& getLabels() const {
        return m_labels;
    }

    virtual ~LabelSet();

    void draw(ShaderProgram& _shader) override {}

    size_t bufferSize() const override { return 0; }

    void setLabels(std::vector<std::unique_ptr<Label>>& _labels);

    void reset();

protected:
    std::vector<std::unique_ptr<Label>> m_labels;
};

}
