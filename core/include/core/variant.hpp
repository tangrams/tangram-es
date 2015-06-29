#ifndef CORE_VARIANT_HPP
#define CORE_VARIANT_HPP

#include <core/type_traits.hpp>
#include <core/functional.hpp>
#include <core/utility.hpp>

#include <stdexcept>
#include <typeinfo>
#include <limits>

#include <cstdlib>
#include <cstdint>

namespace core {
inline namespace v1 {
namespace impl {

/* This is used to get around GCC's inability to expand lambdas in variadic
 * template functions. You make me so sad sometimes, GCC.
 */
template <class Visitor, class Type, class Data, class Result, class... Args>
auto visitor_gen () -> Result {
  return [](Visitor&& visitor, Data* data, Args&&... args) {
    return invoke(
      ::core::forward<Visitor>(visitor),
      *static_cast<Type*>(data),
      ::core::forward<Args>(args)...
    );
  };
}

} /* namespace impl */

#ifndef CORE_NO_EXCEPTIONS
struct bad_variant_get final : ::std::logic_error {
  using ::std::logic_error::logic_error;
};
[[noreturn]] inline void throw_bad_variant_get () {
  throw bad_variant_get { "incorrect type" };
}
#else /* CORE_NO_EXCEPTIONS */
[[noreturn]] inline void throw_bad_variant_get () { ::std::abort(); }
#endif /* CORE_NO_EXCEPTIONS */

/* visitation semantics require that, given a callable type C, and variadic
 * arguments Args... that the return type of the visit will be SFINAE-ified
 * as common_type_t<invoke_of_t<C, Args>...> (this assumes a variadic
 * approach can be taken with common_type, which it cannot at this time. A
 * custom SFINAE-capable version has been written within the type traits
 * component.
 *
 * Obviously if a common type cannot be found, then the visitation function
 * cannot be generated.
 *
 * These same semantics are required for variant<Ts...>::match which simply
 * calls visit with a generate overload<Lambdas...> type.
 */
template <class... Ts>
class variant final {
  using pack_type = meta::pack<Ts...>;

  static_assert(
    pack_type::size() < ::std::numeric_limits<::std::uint8_t>::max(),
    "Cannot have more elements than variant can containe");

  static_assert(
    meta::all<
      meta::boolean<meta::count<Ts, meta::pack<Ts...>>::value == 1>...
    >::value,
    "Cannot have duplicate types in variant");

  static_assert(
    sizeof...(Ts) < ::std::numeric_limits<uint8_t>::max(),
    "Cannot have more elements than variant can contain"
  );

  using storage_type = aligned_storage_t<
    sizeof(impl::discriminate<Ts...>),
    ::std::alignment_of<impl::discriminate<Ts...>>::value
  >;

  template <::std::size_t N> using element = meta::element_t<N, pack_type>;
  template <::std::size_t N> using index = meta::size<N>;

  struct copier final {
    using data_type = add_pointer_t<void>;
    data_type data;

    template <class T>
    void operator ()(T const& value) const { ::new (this->data) T(value); }
  };

  struct mover final {
    using data_type = add_pointer_t<void>;
    data_type data;

    template <class T>
    void operator () (T&& value) {
      ::new (this->data) decay_t<T>(::core::move(value));
    }
  };

  struct destroyer final {
    template <class T> void operator ()(T const& value) const { value.~T(); }
  };

  struct swapper final {
    using data_type = add_pointer_t<void>;
    data_type data;

    template <class T> void operator () (T const&) = delete;

    template <class T>
    void operator ()(T& value) noexcept(is_nothrow_swappable<T>::value) {
      using ::std::swap;
      swap(*static_cast<T*>(this->data), value);
    }
  };

  struct equality final {
    using data_type = add_pointer_t<add_const_t<void>>;
    data_type data;

    template <class T>
    bool operator ()(T const& value) {
      return equal_to<> { }(*static_cast<T const*>(this->data), value);
    }
  };

  struct less_than final {
    using data_type = add_pointer_t<add_const_t<void>>;
    data_type data;

    template <class T>
    bool operator ()(T const& value) noexcept {
      return less<> { }(*static_cast<T const*>(this->data), value);
    }
  };

#ifndef CORE_NO_RTTI
  struct type_info final {
    template <class T>
    ::std::type_info const* operator ()(T&&) const noexcept {
      return ::std::addressof(typeid(decay_t<T>));
    }
  };
#endif /* CORE_NO_RTTI */

  template <
    ::std::size_t N,
    class=enable_if_t<N < sizeof...(Ts)>,
    class T
  > explicit variant (index<N>&&, ::std::false_type&&, T&& value) :
    variant {
      index<N + 1> { },
      ::std::is_constructible<type_at_t<N + 1, Ts...>, T> { },
      ::core::forward<T>(value)
    }
  { }

  template <
    ::std::size_t N,
    class=enable_if_t<N < sizeof...(Ts)>,
    class T
  > explicit variant (index<N>&&, ::std::true_type&&, T&& value) :
    data { }, tag { N }
  { ::new (this->pointer()) type_at_t<N, Ts...> (::core::forward<T>(value)); }

  template <class T>
  using select_index = conditional_t<
    meta::count<decay_t<T>, pack_type>::value,
    meta::index<decay_t<T>, pack_type>,
    index<0>
  >;

public:

  /* The conditional_t used here allows us to first check if a given type
   * is declared in the variant and if it is, we will try to find its
   * constructor and immediately jump there, otherwise, we go the slower
   * route of trying to construct something from the value given.
   *
   * While this route is 'slower' this is a compile time performance issue and
   * will not impact runtime performance.
   *
   * Unfortunately we *do* instantiate templates several times, but there's
   * not much we can do about it.
   */
  template <
    class T,
    class=enable_if_t<not ::std::is_same<decay_t<T>, variant>::value>
  > variant (T&& value) :
    variant {
      select_index<T> { },
      ::std::is_constructible<type_at_t<select_index<T>::value, Ts...>, T> { },
      ::core::forward<T>(value)
    }
  { }

  variant (variant const& that) :
    data { }, tag { that.tag }
  { that.visit(copier { this->pointer() }); }

  variant (variant&& that) noexcept :
    data { }, tag { that.tag }
  { that.visit(mover { this->pointer() }); }

  variant () : variant { type_at_t<0, Ts...> { } } { }

  ~variant () { this->visit(destroyer { }); }

  template <
    class T,
    class=enable_if_t<not ::std::is_same<decay_t<T>, variant>::value>
  > variant& operator = (T&& value) {
    variant { ::core::forward<T>(value) }.swap(*this);
    return *this;
  }

  variant& operator = (variant const& that) {
    variant { that }.swap(*this);
    return *this;
  }

  variant& operator = (variant&& that) noexcept {
    this->visit(destroyer { });
    this->tag = that.tag;
    that.visit(mover { this->pointer() });
    return *this;
  }

  /* Placing these inside of the variant results in no implicit conversions
   * occuring
   */
  bool operator == (variant const& that) const noexcept {
    if (this->tag != that.tag) { return false; }
    return that.visit(equality { this->pointer() });
  }

  bool operator < (variant const& that) const noexcept {
    if (this->tag != that.tag) { return this->tag < that.tag; }
    return that.visit(less_than { this->pointer() });
  }

  void swap (variant& that) noexcept(
    all_traits<is_nothrow_swappable<Ts>...>::value
  ) {
    if (this->which() == that.which()) {
      that.visit(swapper { this->pointer() });
      return;
    }
    variant temp { ::core::move(*this) };
    *this = ::core::move(that);
    that = ::core::move(temp);
  }

  template <class Visitor, class... Args>
  auto visit (Visitor&& visitor, Args&&... args) -> common_type_t<
    invoke_of_t<Visitor, add_lvalue_reference_t<Ts>, Args...>...
  > {
    using return_type = common_type_t<
      invoke_of_t<
        Visitor,
        add_lvalue_reference_t<Ts>,
        Args...
      >...
    >;
    using function = return_type(*)(Visitor&&, void*, Args&&...);
    constexpr ::std::size_t size = pack_type::size();

    static function const callers[size] {
      impl::visitor_gen<Visitor, Ts, void, function, Args...>()...
    };

    return callers[this->tag](
      ::core::forward<Visitor>(visitor),
      this->pointer(),
      ::core::forward<Args>(args)...
    );
  }

  template <class Visitor, class... Args>
  auto visit (Visitor&& visitor, Args&&... args) const -> common_type_t<
    invoke_of_t<Visitor, add_lvalue_reference_t<add_const_t<Ts>>, Args...>...
  > {
    using return_type = common_type_t<
      invoke_of_t<
        Visitor,
        add_lvalue_reference_t<add_const_t<Ts>>,
        Args...
      >...
    >;
    using function = return_type(*)(Visitor&&, void const*, Args&&...);
    constexpr ::std::size_t size = pack_type::size();

    static function const callers[size] = {
      impl::visitor_gen<
        Visitor,
        add_const_t<Ts>,
        void const,
        function,
        Args...
      >()...
    };

    return callers[this->tag](
      ::core::forward<Visitor>(visitor),
      this->pointer(),
      ::core::forward<Args>(args)...
    );
  }

  template <class... Visitors>
  auto match (Visitors&&... visitors) -> decltype(
    this->visit(impl::make_overload(::core::forward<Visitors>(visitors)...))
  ) {
    return this->visit(
      impl::make_overload(::core::forward<Visitors>(visitors)...)
    );
  }

  template <class... Visitors>
  auto match (Visitors&&... visitors) const -> decltype(
    this->visit(impl::make_overload(::core::forward<Visitors>(visitors)...))
  ) {
    return this->visit(
      impl::make_overload(::core::forward<Visitors>(visitors)...)
    );
  }

  /* These functions are undocumented and should not be used outside of core */
  template <::std::size_t N>
  add_pointer_t<add_const_t<element<N>>> cast () const noexcept {
    return static_cast<add_pointer_t<add_const_t<element<N>>>>(this->pointer());
  }

  template <::std::size_t N>
  add_pointer_t<element<N>> cast () noexcept {
    return static_cast<add_pointer_t<element<N>>>(this->pointer());
  }

  template <::std::size_t N>
  auto get () const& noexcept(false) -> element<N> const& {
    if (this->tag != N) { throw_bad_variant_get(); }
    return *static_cast<element<N> const*>(this->pointer());
  }

  template <::std::size_t N>
  auto get () && noexcept(false) -> element<N>&& {
    if (this->tag != N) { throw_bad_variant_get(); }
    return ::core::move(*static_cast<element<N>*>(this->pointer()));
  }

  template <::std::size_t N>
  auto get () & noexcept(false) -> element<N>& {
    if (this->tag != N) { throw_bad_variant_get(); }
    return *static_cast<element<N>*>(this->pointer());
  }


#ifndef CORE_NO_RTTI
  ::std::type_info const& type () const noexcept {
    return *this->visit(type_info { });
  }
#endif /* CORE_NO_RTTI */

  ::std::uint32_t which () const noexcept { return this->tag; }
  bool empty () const noexcept { return false; }

private:
  void const* pointer () const noexcept { return ::std::addressof(this->data); }
  void* pointer () noexcept { return ::std::addressof(this->data); }

  storage_type data;
  ::std::uint8_t tag;
};

template <class... Ts>
void swap (variant<Ts...>& lhs, variant<Ts...>& rhs) noexcept(
  noexcept(lhs.swap(rhs))
) { lhs.swap(rhs); }

template <::std::size_t I, class... Ts>
auto get (variant<Ts...> const* v) noexcept -> enable_if_t<
  I < sizeof...(Ts),
  add_pointer_t<add_const_t<meta::element_t<I, meta::pack<Ts...>>>>
> {
  using t = add_pointer_t<add_const_t<meta::element_t<I, meta::pack<Ts...>>>>;
  return v and v->which() == I
    ? static_cast<t>(v->template cast<I>())
    : nullptr;
}

template <::std::size_t I, class... Ts>
auto get (variant<Ts...>* v) noexcept -> enable_if_t<
  I < sizeof...(Ts),
  add_pointer_t<meta::element_t<I, meta::pack<Ts...>>>
> {
  using t = add_pointer_t<meta::element_t<I, meta::pack<Ts...>>>;
  return v and v->which() == I
    ? static_cast<t>(v->template cast<I>())
    : nullptr;
}

template <::std::size_t I, class... Ts>
auto get (variant<Ts...> const& v) noexcept(false) -> enable_if_t<
  I < sizeof...(Ts),
  add_lvalue_reference_t<add_const_t<meta::element_t<I, meta::pack<Ts...>>>>
> {
  if (auto ptr = get<I>(::std::addressof(v))) { return *ptr; }
  throw_bad_variant_get();
}

template <::std::size_t I, class... Ts>
auto get (variant<Ts...>&& v) noexcept(false) -> enable_if_t<
  I < sizeof...(Ts),
  add_rvalue_reference_t<meta::element_t<I, meta::pack<Ts...>>>
> {
  if (auto p = get<I>(::std::addressof(v))) { return ::core::move(*p); }
  throw_bad_variant_get();
}

template <::std::size_t I, class... Ts>
auto get (variant<Ts...>& v) noexcept(false) -> enable_if_t<
  I < sizeof...(Ts),
  add_lvalue_reference_t<meta::element_t<I, meta::pack<Ts...>>>
> {
  if (auto ptr = get<I>(::std::addressof(v))) { return *ptr; }
  throw_bad_variant_get();
}

template <class T, class... Ts>
auto get (variant<Ts...> const* v) noexcept(false) -> enable_if_t<
  not meta::find_t<T, meta::pack<Ts...>>::empty(),
  decltype(get<meta::index<T, meta::pack<Ts...>>::value>(v))
> { return get<meta::index<T, meta::pack<Ts...>>::value>(v); }

template <class T, class... Ts>
auto get (variant<Ts...>* v) noexcept(false) -> enable_if_t<
  not meta::find_t<T, meta::pack<Ts...>>::empty(),
  decltype(get<meta::index<T, meta::pack<Ts...>>::value>(v))
> { return get<meta::index<T, meta::pack<Ts...>>::value>(v); }

template <class T, class... Ts>
auto get (variant<Ts...> const& v) noexcept(false) -> enable_if_t<
  not meta::find_t<T, meta::pack<Ts...>>::empty(),
  decltype(get<meta::index<T, meta::pack<Ts...>>::value>(v))
> { return get<meta::index<T, meta::pack<Ts...>>::value>(v); }

template <class T, class... Ts>
auto get (variant<Ts...>&& v) noexcept(false) -> enable_if_t<
  not meta::find_t<T, meta::pack<Ts...>>::empty(),
  decltype(core::move(get<meta::index<T, meta::pack<Ts...>>::value>(v)))
> { return core::move(get<meta::index<T, meta::pack<Ts...>>::value>(v)); }

template <class T, class... Ts>
auto get (variant<Ts...>& v) noexcept(false) -> enable_if_t<
  not meta::find_t<T, meta::pack<Ts...>>::empty(),
  decltype(get<meta::index<T, meta::pack<Ts...>>::value>(v))
> { return get<meta::index<T, meta::pack<Ts...>>::value>(v); }

}} /* namespace core::v1 */

namespace std {

template <class... Ts>
struct hash<core::v1::variant<Ts...>> {
  using argument_type = core::v1::variant<Ts...>;
  using result_type = size_t;
  result_type operator () (argument_type const& value) const {
    return value.match(hash<Ts> { }...);
  }
};

template <size_t I, class... Ts>
[[gnu::deprecated]]
auto get (::core::v1::variant<Ts...> const& v) noexcept(false) -> decltype(
  ::core::v1::get<I>(v)
) { return ::core::v1::get<I>(v); }

template <size_t I, class... Ts>
[[gnu::deprecated]]
auto get (::core::v1::variant<Ts...>&& v) noexcept(false) -> decltype(
  ::core::v1::get<I>(v)
) { return ::core::v1::get<I>(v); }

template <size_t I, class... Ts>
[[gnu::deprecated]]
auto get (::core::v1::variant<Ts...>& v) noexcept (false) -> decltype(
  ::core::v1::get<I>(v)
) { return ::core::v1::get<I>(v); }

} /* namespace std */

#endif /* CORE_VARIANT_HPP */
