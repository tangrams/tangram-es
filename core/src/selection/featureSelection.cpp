#include "selection/featureSelection.h"

namespace Tangram {

FeatureSelection::FeatureSelection() :
    m_entry(0) {
}

uint32_t FeatureSelection::nextColorIdentifier() {

    uint32_t entry = m_entry++;

    // skip zero every 2^32 features
    while (entry == 0) {
        entry = m_entry++;
    }

    return entry;
}

}
