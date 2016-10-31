#include "featureSelection.h"

namespace Tangram {

FeatureSelection::FeatureSelection() :
    m_entry(0) {
}

uint32_t FeatureSelection::colorIdentifier() {

    uint32_t entry;

    // skip zero, used for non-selectable features
    // skip one, used for non-selectable but occluding features
    do  {
        entry = m_entry++;
    } while (entry < 2);

    return entry;
}

}
