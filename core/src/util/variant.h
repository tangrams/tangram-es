#pragma once

#include "variant/variant.hpp"

namespace Tangram {
    struct none_type {};

    template<typename... Types>
    using variant = mapbox::util::variant<Types...>;
}
