#include "labels/labelSet.h"

namespace Tangram {

LabelSet::~LabelSet() {}

void LabelSet::reset() {
    for (auto& label : m_labels) {
        label->resetState();
    }
}

void LabelSet::setLabels(std::vector<std::unique_ptr<Label>>& _labels) {
    typedef std::vector<std::unique_ptr<Label>>::iterator iter_t;
    m_labels.clear();
    m_labels.insert(m_labels.end(),
                    std::move_iterator<iter_t>(_labels.begin()),
                    std::move_iterator<iter_t>(_labels.end()));

    _labels.clear();
}

}
