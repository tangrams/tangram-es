#include "tileData.h"

namespace Tangram {
static Properties::Value NOT_FOUND = Properties::none_type{};

const Properties::Value& Properties::get(const std::string& key) const {

    auto it = props.find(key);
    if (it == props.end()) {
        return NOT_FOUND;
    }
    return it->second;
}

}
