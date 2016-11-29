#pragma once

// Always compile variant with NDEBUG
// to properly inline functions
#ifdef NDEBUG
#define KEEP_NDEBUG
#else
#define NDEBUG
#endif

#include "variant/include/mapbox/variant.hpp"

#ifndef KEEP_NDEBUG
#undef NDEBUG
#endif

#include <string>

namespace Tangram {
// Primarily used when duk evaluated jsFunction results in a a null or undefined value
struct Undefined {
    template<typename T>
    bool operator==(T const& rhs) const {
        return false;
    }
    bool operator==(Undefined const& rhs) const {
        return true;
    }
    bool operator<(Undefined const& rhs) const {
        return false;
    }
};

struct none_type {
    template<typename T>
    bool operator==(T const& rhs) const {
        return false;
    }
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
using Value = variant<none_type, double, std::string>;
}

class Value : public detail::Value {
    using Base = detail::Value;
    using Base::Base;
};

const static Value NOT_A_VALUE(none_type{});

}
