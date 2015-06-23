#ifndef FLYWEIGHT_EXTRACTOR_HPP
#define FLYWEIGHT_EXTRACTOR_HPP

#include <core/type_traits.hpp>

namespace flyweight {
inline namespace v1 {

template <class T>
struct extractor {
  constexpr extractor () noexcept { }

  T const& operator () (T const& argument) const noexcept { return argument; }
};

}} /* namespace flyweight::v1 */

#endif /* FLYWEIGHT_EXTRACTOR_HPP */
