#pragma once

#include <vector>
#include <memory>
#include "label.h"

namespace Tangram {

class LabelSet {
public:
    const std::vector<std::unique_ptr<Label>>& getLabels() const {
        return m_labels;
    }

    virtual ~LabelSet();

    void setLabels(std::vector<std::unique_ptr<Label>>& _labels);

    void reset();

protected:
    std::vector<std::unique_ptr<Label>> m_labels;
};

}
