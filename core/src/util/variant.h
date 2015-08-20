#pragma once

#include "variant/variant.hpp"

namespace Tangram {
struct none_type {
    bool operator==(none_type const& rhs) const {
        return true;
    }
    bool operator<(none_type const& rhs) const {
        return false;
    }
};

template<typename... Types>
using variant = mapbox::util::variant<Types...>;


/* Common Value type for Feature Properties and Filter Values */
using Value = variant<none_type, std::string, float>;

}
