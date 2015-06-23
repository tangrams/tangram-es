#ifndef CORE_META_HPP
#define CORE_META_HPP

#include <type_traits>
#include <limits>
#include <tuple>

#include <cstdint>
#include <cstddef>

namespace core {
namespace meta {
inline namespace v1 {

template <class T> struct identity { using type = T; };

template <class T, T... I> struct integer_sequence : identity<T> {
  static_assert(
    ::std::is_integral<T>::value,
    "integer_sequence must use an integral type"
  );

  template <T N> using append = integer_sequence<T, I..., N>;
  static constexpr ::std::size_t size() noexcept { return sizeof...(I); }
  using next = append<size()>;
};

template <class T, T Index, ::std::size_t N>
struct iota : identity<
  typename iota<T, Index - 1, N - 1u>::type::next
> { static_assert(Index >= 0, "Index cannot be negative"); };

template <class T, T Index>
struct iota<T, Index, 0u> : identity<integer_sequence<T>> { };

template <::std::size_t... I>
using index_sequence = integer_sequence<::std::size_t, I...>;

template <class T, T N>
using make_integer_sequence = typename iota<T, N, N>::type;

template <::std::size_t N>
using make_index_sequence = make_integer_sequence<::std::size_t, N>;

template <class... Ts>
using index_sequence_for = make_index_sequence<sizeof...(Ts)>;

template <::std::size_t I>
using size = ::std::integral_constant<::std::size_t, I>;

template <bool B> using boolean = ::std::integral_constant<bool, B>;
template <int I> using integer = ::std::integral_constant<int, I>;

template <class...> struct all;
template <class T, class... Args>
struct all<T, Args...> : boolean<T::value and all<Args...>::value> { };
template <> struct all<> : ::std::true_type { };

template <class...> struct any;
template <class T, class... Args>
struct any<T, Args...> : boolean<T::value or any<Args...>::value> { };
template <> struct any<> : ::std::false_type { };

template <class... Args> using none = boolean<not all<Args...>::value>;

/* type list forward declarations */
template <class, template <class> class> struct transform;
template <class, template <class> class> struct count_if;
template <class, template <class> class> struct find_if;
template <class, template <class> class> struct filter;
template <::std::size_t, class> struct element;

template <class, class...> struct push_front;
template <class, class...> struct push_back;
template <class, class> struct index;
template <class, class> struct count;
template <class, class> struct find;

template <class...> struct merge;

template <class> struct index_sequence_from;
template <class> struct pop_front;
template <class> struct pop_back;
template <class> struct reverse;
template <class> struct to_pack;

template <class, template <class...> class> struct from_pack;

template <class...> struct pack;

/* type aliases */
template <class T, template <class> class F>
using transform_t = typename transform<T, F>::type;

template <class T, template <class> class F>
using find_if_t = typename find_if<T, F>::type;

template <class T, template <class> class F>
using filter_t = typename filter<T, F>::type;

template <::std::size_t N, class T>
using element_t = typename element<N, T>::type;

template <class T, class... Us>
using push_front_t = typename push_front<T, Us...>::type;

template <class T, class... Us>
using push_back_t = typename push_back<T, Us...>::type;

template <class T, class V> using find_t = typename find<T, V>::type;

template <class... Ts> using merge_t = typename merge<Ts...>::type;

template <class T> using pop_front_t = typename pop_front<T>::type;
template <class T> using pop_back_t = typename pop_back<T>::type;
template <class T> using reverse_t = typename reverse<T>::type;

template <class T, template <class...> class U>
using from_pack_t = typename from_pack<T, U>::type;

template <> struct pack<> {
  static constexpr ::std::size_t size() noexcept { return 0u; }
  static constexpr bool empty () noexcept { return false; }
};

template <class... Ts>
struct pack {
  static constexpr ::std::size_t size () noexcept { return sizeof...(Ts); }
  static constexpr bool empty () noexcept { return size() == 0; }

  using front = element_t<0, pack>;
  using back = element_t<size() - 1u, pack>;
};

template <class... Ts, template <class> class F>
struct transform<pack<Ts...>, F> :
  identity<pack<F<Ts>...>>
{ };

template <class... Ts, template <class> class F>
struct count_if<pack<Ts...>, F> :
  meta::size<filter_t<pack<Ts...>, F>::size()>
{ };

template <template <class> class F>
struct find_if<pack<>, F> : identity<pack<>> { };

template <class T, class... Ts, template <class> class F>
struct find_if<pack<T, Ts...>, F> : identity<
  typename ::std::conditional<
    F<T>::value,
    pack<T, Ts...>,
    find_if_t<pack<Ts...>, F>
  >::type
> { };

template <class... Ts, template <class> class F>
struct filter<pack<Ts...>, F> :
  merge<
    typename ::std::conditional<
      F<Ts>::value,
      pack<Ts>,
      pack<>
    >::type...
  >
{ };

template <class T, class... Ts>
struct element<0u, pack<T, Ts...>> :
  identity<T>
{ };

template <::std::size_t N, class T, class... Ts>
struct element<N, pack<T, Ts...>> :
  element<N - 1, pack<Ts...>>
{ static_assert(N < (sizeof...(Ts) + 1), "given index is out of range"); };

template <class... Ts, class... Us>
struct push_front<pack<Ts...>, Us...> : identity<
  pack<Us..., Ts...>
> { };

template <class... Ts, class... Us>
struct push_back<pack<Ts...>, Us...> : identity<
  pack<Ts..., Us...>
> { };

template <class T> struct is {
  template <class U> using convertible = ::std::is_convertible<T, U>;
  template <class U> using assignable = ::std::is_assignable<T, U>;
  template <class U> using base_of = ::std::is_base_of<T, U>;
  template <class U> using same = ::std::is_same<T, U>;

  template <class U>
  using nothrow_assignable = ::std::is_nothrow_assignable<T, U>;
};

template <class T, class... Ts>
struct count<T, pack<Ts...>> :
  count_if<pack<Ts...>, is<T>::template same>
{ };

template <class T, class... Ts>
struct index<T, pack<Ts...>> : ::std::conditional<
  find_t<T, pack<Ts...>>::empty(),
  meta::size<::std::numeric_limits<::std::size_t>::max()>,
  meta::size<pack<Ts...>::size() - find_t<T, pack<Ts...>>::size()>
>::type { };

template <class T, class... Ts>
struct find<T, pack<Ts...>> :
  find_if<pack<Ts...>, is<T>::template same>
{ };

template <class... Ts>
struct merge<pack<Ts...>> : identity<pack<Ts...>> { };

template <class... Ts, class... Us, class... Vs>
struct merge<pack<Ts...>, pack<Us...>, Vs...> :
  merge<pack<Ts..., Us...>, Vs...>
{ };

template <class... Ts> struct index_sequence_from<pack<Ts...>> :
  index_sequence_for<Ts...>
{ };

template <class T, class... Ts> struct pop_front<pack<T, Ts...>> :
  identity<pack<Ts...>>
{ };

template <class... Ts> struct pop_back<pack<Ts...>> :
  reverse<pop_front_t<reverse_t<pack<Ts...>>>>
{ };

template <class... Ts> struct reverse<pack<Ts...>> {
  template <class T> struct impl;
  template <::std::size_t... I>
  struct impl<index_sequence<I...>> : identity<
    pack<element_t<sizeof...(I) - I - 1u, pack<Ts...>>...>
  > { };
  using type = typename impl<index_sequence_for<Ts...>>::type;
};

template <template <class...> class T, class... Ts>
struct to_pack<T<Ts...>> : identity<pack<Ts...>> { };

template <class... Ts, template <class...> class To>
struct from_pack<pack<Ts...>, To> : identity<
  To<Ts...>
> { };

}}} /* namespace core::meta::v1 */

#endif /* CORE_META_HPP */
