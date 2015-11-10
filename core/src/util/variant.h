#pragma once

// Always compile variant with NDEBUG
// to properly inline functions
#ifdef NDEBUG
#define KEEP_NDEBUG
#else
#define NDEBUG
#endif

#include "glm/glm.hpp"
#include "variant/variant.hpp"

#ifndef KEEP_NDEBUG
#undef NDEBUG
#endif

#include <string>

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

namespace detail {
/* Common Value type for Feature Properties and Filter Values */
using Value = variant<none_type, float, int64_t, std::string>;
}

class Value : public detail::Value {
    using Base = detail::Value;
    using Base::Base;
};

/* Style Block Uniform types */
using UniformValue = variant<none_type, bool, std::string, float, int, glm::vec2, glm::vec3, glm::vec4, glm::mat2, glm::mat3, glm::mat4>;

}
