#ifndef CORE_FUNCTIONAL_HPP
#define CORE_FUNCTIONAL_HPP

#include <functional>
#include <tuple>
#include <array>

#include <core/type_traits.hpp>
#include <core/utility.hpp>

namespace core {
inline namespace v1 {

template <class T> struct is_reference_wrapper : ::std::false_type { };
template <class T>
struct is_reference_wrapper<::std::reference_wrapper<T>> :
  ::std::true_type
{ };

template <class T>
struct is_reference_wrapper<::std::reference_wrapper<T> const> :
  ::std::true_type
{ };

template <class T>
struct is_reference_wrapper<::std::reference_wrapper<T> volatile> :
  ::std::true_type
{ };

template <class T>
struct is_reference_wrapper<::std::reference_wrapper<T> const volatile> :
  ::std::true_type
{ };

template <class F> struct function_traits;

template <class R, class... Args>
struct function_traits<R(*)(Args...)> : function_traits<R(Args...)> { };

template <class C, class R>
struct function_traits<R(C::*)> : function_traits<R(C&)> { };

template <class C, class R, class... Args>
struct function_traits<R(C::*)(Args...)> : function_traits<R(C&, Args...)> { };

template <class C, class R, class... Args>
struct function_traits<R(C::*)(Args...) const volatile> :
  function_traits<R(C volatile const&, Args...)>
{ };

template <class C, class R, class... Args>
struct function_traits<R(C::*)(Args...) volatile> :
  function_traits<R(C volatile&, Args...)>
{ };

template <class C, class R, class... Args>
struct function_traits<R(C::*)(Args...) const> :
  function_traits<R(C const&, Args...)>
{ };

template <class R, class... Args>
struct function_traits<R(Args...)> {
  using return_type = R;

  using pointer = return_type(*)(Args...);
  static constexpr ::std::size_t arity = sizeof...(Args);

  template < ::std::size_t N>
  using argument = typename ::std::tuple_element<
    N,
    ::std::tuple<Args...>
  >::type;
};

template <class F> struct function_traits {
  using functor_type = function_traits<decltype(&decay_t<F>::operator())>;
  using return_type = typename functor_type::return_type;
  using pointer = typename functor_type::pointer;
  static constexpr ::std::size_t arity = functor_type::arity - 1;
  template <::std::size_t N>
  using argument = typename functor_type::template argument<N>;
};

/* N3727 */
template <class Functor, class... Args>
auto invoke (Functor&& f, Args&&... args) -> enable_if_t<
  ::std::is_member_pointer<decay_t<Functor>>::value,
  result_of_t<Functor&&(Args&&...)>
> { return ::std::mem_fn(f)(core::forward<Args>(args)...); }

template <class Functor, class... Args>
constexpr auto invoke (Functor&& f, Args&&... args) -> enable_if_t<
  not ::std::is_member_pointer<decay_t<Functor>>::value,
  result_of_t<Functor&&(Args&&...)>
> { return core::forward<Functor>(f)(core::forward<Args>(args)...); }

namespace impl {

template <class F, class T, ::std::size_t... I>
auto apply (F&& f, T&& t, index_sequence<I...>) -> decltype(
  invoke(core::forward<F>(f), ::std::get<I>(core::forward<T>(t))...)
) { return invoke(core::forward<F>(f), ::std::get<I>(core::forward<T>(t))...); }

} /* namespace impl */

template <
  class Functor,
  class T,
  class I = make_index_sequence<::std::tuple_size<decay_t<T>>::value>
> auto apply (Functor&& f, T&& t) -> decltype(
  impl::apply(core::forward<Functor>(f), core::forward<T>(t), I())
) { return impl::apply(core::forward<Functor>(f), core::forward<T>(t), I()); }


namespace impl {

template <class U, ::std::size_t... I>
auto unpack (U&& u, index_sequence<I...>) -> decltype(
  core::invoke(::std::get<I>(core::forward<U>(u))...)
) { return core::invoke(::std::get<I>(core::forward<U>(u))...); }

template <class F, class U, ::std::size_t... I>
auto runpack (F&& f, U&& u, index_sequence<I...>) -> decltype(
  core::invoke(core::forward<F>(f), core::forward<U>(u).at(I)...)
) { return core::invoke(core::forward<F>(f), core::forward<U>(u).at(I)...); }

} /* namespace impl */

struct unpack_t final { };
constexpr unpack_t unpack { };

struct runpack_t final { };
constexpr runpack_t runpack { };

/* Use apply instead */
template <
  class F,
  class U,
  class I = make_index_sequence<::std::tuple_size<decay_t<U>>::value>
> [[gnu::deprecated]] auto invoke (unpack_t, F&& f, U&& u) -> enable_if_t<
  is_unpackable<decay_t<U>>::value,
  decltype(impl::apply(core::forward<F>(f), core::forward<U>(u), I { }))
> { return impl::apply(core::forward<F>(f), core::forward<U>(u), I { }); }

template <
  class U,
  class I = make_index_sequence<::std::tuple_size<decay_t<U>>::value>
> [[gnu::deprecated]] auto invoke (unpack_t, U&& u) -> enable_if_t<
  is_unpackable<decay_t<U>>::value,
  decltype(impl::unpack(core::forward<U>(u), I { }))
> { return impl::unpack(core::forward<U>(u), I { }); }

/* Modified to force clang to *not* select this function in a bizarre corner
 * case.
 */
template <
  class F,
  class R,
  class=enable_if_t<is_runpackable<decay_t<R>>::value>,
  class I = make_index_sequence<function_traits<F>::arity>
> auto invoke (runpack_t, F&& f, R&& r) -> decltype(
  impl::runpack(core::forward<F>(f), core::forward<R>(r), I { })
) { return impl::runpack(core::forward<F>(f), core::forward<R>(r), I { }); }

template <class F>
struct apply_functor {
  explicit apply_functor (F&& f) : f(core::forward<F>(f)) { }

  template <class Applicable>
  auto operator () (Applicable&& args) -> decltype(
    core::apply(core::forward<F>(this->f), core::forward<Applicable>(args))
  ) { return apply(core::forward<F>(f), core::forward<Applicable>(args)); }
private:
  F f;
};

template <class F>
auto make_apply (F&& f) -> apply_functor<F> {
  return apply_functor<F> { core::forward<F>(f) };
}

/* function objects -- arithmetic */
template <class T=void>
struct plus : impl::binary<T, T, T> {
  constexpr T operator () (T const& l, T const& r) const { return l + r; }
};

template <class T=void>
struct minus : impl::binary<T, T, T> {
  constexpr T operator () (T const& l, T const& r) const { return l - r; }
};

template <class T=void>
struct multiplies : impl::binary<T, T, T> {
  constexpr T operator () (T const& l, T const& r) const { return l * r; }
};

template <class T=void>
struct divides : impl::binary<T, T, T> {
  constexpr T operator () (T const& l, T const& r) const { return l / r; }
};

template <class T=void>
struct modulus : impl::binary<T, T, T> {
  constexpr T operator () (T const& l, T const& r) const { return l % r; }
};

template <class T=void>
struct negate : impl::unary<T, T> {
  constexpr T operator () (T const& arg) const { return -arg; }
};

/* function objects -- comparisons */
template <class T=void>
struct equal_to : impl::binary<T, T, bool> {
  constexpr bool operator () (T const& l, T const& r) const { return l == r; }
};

template <class T=void>
struct not_equal_to : impl::binary<T, T, bool> {
  constexpr bool operator () (T const& l, T const& r) const { return l != r; }
};

template <class T=void>
struct greater_equal : impl::binary<T, T, bool> {
  constexpr bool operator () (T const& l, T const& r) const { return l >= r; }
};

template <class T=void>
struct less_equal : impl::binary<T, T, bool> {
  constexpr bool operator () (T const& l, T const& r) const { return l <= r; }
};

template <class T=void>
struct greater : impl::binary<T, T, bool> {
  constexpr bool operator () (T const& l, T const& r) const { return l > r; }
};

template <class T=void>
struct less : impl::binary<T, T, bool> {
  constexpr bool operator () (T const& l, T const& r) const { return l < r; }
};

/* function objects -- logical */
template <class T=void>
struct logical_and : impl::binary<T, T, bool> {
  constexpr bool operator () (T const& l, T const& r) const { return l and r; }
};

template <class T=void>
struct logical_or : impl::binary<T, T, bool> {
  constexpr bool operator () (T const& l, T const& r) const { return l or r; }
};

template <class T=void>
struct logical_not : impl::unary<T, bool>  {
  constexpr bool operator () (T const& arg) const { return not arg; }
};

/* function objects -- bitwise */

template <class T=void>
struct bit_and : impl::binary<T, T, T> {
  constexpr bool operator () (T const& l, T const& r) const { return l & r; }
};

template <class T=void>
struct bit_or : impl::binary<T, T, T> {
  constexpr bool operator () (T const& l, T const& r) const { return l | r; }
};

template <class T=void>
struct bit_xor : impl::binary<T, T, T> {
  constexpr bool operator () (T const& l, T const& r) const { return l ^ r; }
};

template <class T=void>
struct bit_not : impl::unary<T, T> {
  constexpr bool operator () (T const& arg) const { return ~arg; }
};

/* function objects -- arithmetic specializations */
template <> struct plus<void> {
  using is_transparent = void;

  template <class T, class U>
  constexpr auto operator () (T&& t, U&& u) const -> decltype(
    core::forward<T>(t) + core::forward<U>(u)
  ) { return core::forward<T>(t) + core::forward<U>(u); }
};

template <> struct minus<void> {
  using is_transparent = void;

  template <class T, class U>
  constexpr auto operator () (T&& t, U&& u) const -> decltype(
    core::forward<T>(t) - core::forward<U>(u)
  ) { return core::forward<T>(t) - core::forward<U>(u); }
};

template <> struct multiplies<void> {
  using is_transparent = void;

  template <class T, class U>
  constexpr auto operator () (T&& t, U&& u) const -> decltype(
    core::forward<T>(t) * core::forward<U>(u)
  ) { return core::forward<T>(t) * core::forward<U>(u); }
};

template <> struct divides<void> {
  using is_transparent = void;

  template <class T, class U>
  constexpr auto operator () (T&& t, U&& u) const -> decltype(
    core::forward<T>(t) / core::forward<U>(u)
  ) { return core::forward<T>(t) / core::forward<U>(u); }
};

template <> struct modulus<void> {
  using is_transparent = void;

  template <class T, class U>
  constexpr auto operator () (T&& t, U&& u) const -> decltype(
    core::forward<T>(t) % core::forward<U>(u)
  ) { return core::forward<T>(t) % core::forward<U>(u); }
};

template <> struct negate<void> {
  using is_transparent = void;

  template <class T>
  constexpr auto operator () (T&& t) const -> decltype(core::forward<T>(t)) {
    return core::forward<T>(t);
  }
};

/* function objects -- comparison specialization */
template <> struct equal_to<void> {
  using is_transparent = void;

  template <class T, class U>
  constexpr auto operator () (T&& t, U&& u) const -> decltype(
    core::forward<T>(t) == core::forward<U>(u)
  ) { return core::forward<T>(t) == core::forward<U>(u); }
};

template <> struct not_equal_to<void> {
  using is_transparent = void;

  template <class T, class U>
  constexpr auto operator () (T&& t, U&& u) const -> decltype(
    core::forward<T>(t) != core::forward<U>(u)
  ) { return core::forward<T>(t) != core::forward<U>(u); }
};

template <> struct greater_equal<void> {
  using is_transparent = void;

  template <class T, class U>
  constexpr auto operator () (T&& t, U&& u) const -> decltype(
    core::forward<T>(t) >= core::forward<U>(u)
  ) { return core::forward<T>(t) >= core::forward<U>(u); }
};

template <> struct less_equal<void> {
  using is_transparent = void;

  template <class T, class U>
  constexpr auto operator () (T&& t, U&& u) const -> decltype(
    core::forward<T>(t) <= core::forward<U>(u)
  ) { return core::forward<T>(t) <= core::forward<U>(u); }
};

template <> struct greater<void> {
  using is_transparent = void;

  template <class T, class U>
  constexpr auto operator () (T&& t, U&& u) const -> decltype(
    core::forward<T>(t) > core::forward<U>(u)
  ) { return core::forward<T>(t) > core::forward<U>(u); }
};

template <> struct less<void> {
  using is_transparent = void;

  template <class T, class U>
  constexpr auto operator () (T&& t, U&& u) const -> decltype(
    core::forward<T>(t) < core::forward<U>(u)
  ) { return core::forward<T>(t) < core::forward<U>(u); }
};

/* function objects -- logical specializations */
template <> struct logical_and<void> {
  using is_transparent = void;

  template <class T, class U>
  constexpr auto operator () (T&& t, U&& u) const -> decltype(
    core::forward<T>(t) and core::forward<U>(u)
  ) { return core::forward<T>(t) and core::forward<U>(u); }
};

template <> struct logical_or<void> {
  using is_transparent = void;

  template <class T, class U>
  constexpr auto operator () (T&& t, U&& u) const -> decltype(
    core::forward<T>(t) or core::forward<U>(u)
  ) { return core::forward<T>(t) or core::forward<U>(u); }
};

template <> struct logical_not<void> {
  using is_transparent = void;

  template <class T>
  constexpr auto operator () (T&& t) const -> decltype(
    not core::forward<T>(t)
  ) { return not core::forward<T>(t); }
};

/* function objects -- bitwise specializations */
template <> struct bit_and<void> {
  using is_transparent = void;

  template <class T, class U>
  constexpr auto operator () (T&& t, U&& u) const -> decltype(
    core::forward<T>(t) & core::forward<U>(u)
  ) { return core::forward<T>(t) & core::forward<U>(u); }
};

template <> struct bit_or<void> {
  using is_transparent = void;

  template <class T, class U>
  constexpr auto operator () (T&& t, U&& u) const -> decltype(
    core::forward<T>(t) | core::forward<U>(u)
  ) { return core::forward<T>(t) | core::forward<U>(u); }
};

template <> struct bit_xor<void> {
  using is_transparent = void;

  template <class T, class U>
  constexpr auto operator () (T&& t, U&& u) const -> decltype(
    core::forward<T>(t) ^ core::forward<U>(u)
  ) { return core::forward<T>(t) ^ core::forward<U>(u); }
};

template <> struct bit_not<void> {
  using is_transparent = void;

  template <class T>
  constexpr auto operator () (T&& t) const -> decltype(~core::forward<T>(t)) {
    return ~core::forward<T>(t);
  }
};

}} /* namespace core::v1 */

#endif /* CORE_FUNCTIONAL_HPP */
