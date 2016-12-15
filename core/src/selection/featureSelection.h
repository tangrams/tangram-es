#pragma once

#include <atomic>

namespace Tangram {

class FeatureSelection {

public:

    FeatureSelection();

    uint32_t nextColorIdentifier();

private:

    std::atomic<uint32_t> m_entry;

};

}
