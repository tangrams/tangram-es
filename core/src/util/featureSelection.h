#pragma once

#include <atomic>

namespace Tangram {

class FeatureSelection {

public:

    FeatureSelection();

    uint32_t colorIdentifier();

private:

    std::atomic<uint32_t> m_entry;

};

}
