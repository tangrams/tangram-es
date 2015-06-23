#ifndef CORE_ARRAY_HPP
#define CORE_ARRAY_HPP

#include <array>

#include <core/type_traits.hpp>
#include <core/functional.hpp>
#include <core/utility.hpp>

namespace core {
inline namespace v1 {

template <class V = void, class... Args>
constexpr auto make_array (Args&&... args) -> ::std::array<
  conditional_t<
    meta::all<
      ::std::is_void<V>,
      meta::none<is_reference_wrapper<Args>...>
    >::value,
    common_type_t<Args...>,
    V
  >,
  sizeof...(Args)
> { return {{ core::forward<Args>(args)... }}; }

template <class T, ::std::size_t N, ::std::size_t... Is>
constexpr auto to_array (T (&array)[N], index_sequence<Is...>) -> ::std::array<
  remove_cv_t<T>, N
> { return {{ array[Is]... }}; }

template <class T, ::std::size_t N>
constexpr auto to_array (T (&array)[N]) -> ::std::array<remove_cv_t<T>, N> {
  return core::to_array(array, make_index_sequence<N> { });
}

}} /* namespace core::v1 */

#endif /* CORE_ARRAY_HPP */
