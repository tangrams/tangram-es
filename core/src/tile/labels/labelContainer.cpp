#include "labelContainer.h"

LabelContainer::LabelContainer() {}

LabelContainer::~LabelContainer() {}

std::shared_ptr<Label> LabelContainer::addLabel(LabelTransform _transform, std::string _text) {
    auto currentBuffer = m_ftContext->getCurrentBuffer();

    if (currentBuffer) {
        std::shared_ptr<Label> label(new Label(_transform, _text, currentBuffer));
        m_labels.insert(label);

        return label;
    } 

    return nullptr;
}
