#ifndef CORE_UTILITY_HPP
#define CORE_UTILITY_HPP

#include <functional>

#include <cstddef>

#include <core/type_traits.hpp>

namespace core {
inline namespace v1 {

template <class T>
constexpr T&& forward (remove_reference_t<T>& t) noexcept {
  return static_cast<T&&>(t);
}

template <class T>
constexpr T&& forward (remove_reference_t<T>&& t) noexcept {
  return static_cast<T&&>(t);
}

template <class T>
constexpr auto move (T&& t) noexcept -> decltype(
  static_cast<remove_reference_t<T>&&>(t)
) { return static_cast<remove_reference_t<T>&&>(t); }


template <class T, T... I>
using integer_sequence = meta::integer_sequence<T, I...>;

template <::std::size_t... I>
using index_sequence = integer_sequence<::std::size_t, I...>;

template <class T, T N>
using make_integer_sequence = typename meta::iota<T, N, N>::type;

template <::std::size_t N>
using make_index_sequence = make_integer_sequence<::std::size_t, N>;

template <class... Ts>
using index_sequence_for = make_index_sequence<sizeof...(Ts)>;

/* N3761 (with some additions) */
template <::std::size_t N, class... Ts>
struct type_at : meta::element<N, meta::pack<Ts...>> { };

template <::std::size_t N, class... Ts>
using type_at_t = typename type_at<N, Ts...>::type;

template <::std::size_t N, class T, class... Ts>
constexpr auto value_at (T&& value, Ts&&...) -> enable_if_t<
  N == 0 and N < (sizeof...(Ts) + 1),
  decltype(::core::forward<T>(value))
> { return ::core::forward<T>(value); }

template <::std::size_t N, class T, class... Ts>
constexpr auto value_at (T&&, Ts&&... values) -> enable_if_t<
  N != 0 and N < (sizeof...(Ts) + 1),
  type_at_t<N, T, Ts...>
> { return value_at<N - 1, Ts...>(::core::forward<Ts>(values)...); }

template <class Callable>
struct scope_guard final {

  static_assert(
    ::std::is_nothrow_move_constructible<Callable>::value,
    "Given type must be nothrow move constructible"
  );

  explicit scope_guard (Callable callable) noexcept :
    callable { ::core::move(callable) },
    dismissed { false }
  { }

  scope_guard (scope_guard const&) = delete;
  scope_guard (scope_guard&&) = default;
  scope_guard () = delete;
  ~scope_guard () noexcept { if (not this->dismissed) { callable(); } }

  scope_guard& operator = (scope_guard const&) = delete;
  scope_guard& operator = (scope_guard&&) = default;

  void dismiss () noexcept { this->dismissed = true; }

private:
  Callable callable;
  bool dismissed;
};

template <class Callable>
auto make_scope_guard(Callable&& callable) -> scope_guard<decay_t<Callable>> {
  return scope_guard<decay_t<Callable>> {
    ::core::forward<Callable>(callable)
  };
}

template <class T, class U=T>
T exchange (T& obj, U&& value) noexcept(
  meta::all<
    ::std::is_nothrow_move_constructible<T>,
    ::std::is_nothrow_assignable<add_lvalue_reference_t<T>, U>
  >::value
) {
  T old = ::core::move(obj);
  obj = ::core::forward<U>(value);
  return old;
}

template <class E>
constexpr auto to_integral(E e) noexcept -> enable_if_t<
  std::is_enum<E>::value,
  underlying_type_t<E>
> { return static_cast<underlying_type_t<E>>(e); }

template <class T>
struct capture final {
  static_assert(::std::is_move_constructible<T>::value, "T must be movable");
  using value_type = T;
  using reference = add_lvalue_reference_t<value_type>;
  using pointer = add_pointer_t<value_type>;

  capture (T&& data) : data { core::move(data) } { }

  capture (capture&&) = default;
  capture (capture& that) : data { core::move(that.data) } { }
  capture () = delete;

  capture& operator = (capture const&) = delete;
  capture& operator = (capture&&) = delete;

  operator reference () const noexcept { return this->get(); }
  reference operator * () const noexcept { return this->get(); }
  pointer operator -> () const noexcept {
    return ::std::addressof(this->get());
  }

  reference get () const noexcept { return this->data; }

private:
  value_type data;
};

template <class T>
auto make_capture (remove_reference_t<T>& ref) -> capture<T> {
  return capture<T> { core::move(ref) };
}

template <class T>
auto make_capture (remove_reference_t<T>&& ref) -> capture<T> {
  return capture<T> { core::move(ref) };
}

}} /* namespace core::v1 */

#endif /* CORE_UTILITY_HPP */
