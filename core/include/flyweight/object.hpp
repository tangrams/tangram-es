#ifndef FLYWEIGHT_OBJECT_HPP
#define FLYWEIGHT_OBJECT_HPP

#include <memory>

#include <flyweight/extractor.hpp>
#include <flyweight/cache.hpp>

namespace flyweight {
inline namespace v1 {

template <
  class T,
  class KeyExtractor=extractor<T>,
  class Allocator=std::allocator<T>,
  class Tag=tag<>
> struct object final {

  using cache_type = cache<T, KeyExtractor, Allocator, Tag>;

  using value_type = core::add_const_t<T>;
  using key_type = typename cache_type::key_type;


  static_assert(
    not std::is_reference<value_type>::value,
    "Cannot have a reference for a value_type"
  );

  using reference = core::add_lvalue_reference_t<value_type>;
  using pointer = core::add_pointer_t<value_type>;

  template <
    class... Args,
    class=core::enable_if_t<
      core::all_traits<
        core::no_traits<std::is_same<core::decay_t<Args>, object>...>,
        std::is_constructible<value_type, Args...>
      >::value
    >
  > object (Args&&... args) : object {
    value_type { ::std::forward<Args>(args)... }
  } { }

  template <
    class ValueType,
    class=core::enable_if_t<
      std::is_same<core::decay_t<ValueType>, core::decay_t<value_type>>::value
    >
  > explicit object (ValueType&& value) : handle {
    cache_type::ref().find(::std::forward<ValueType>(value))
  } { }

  object (object const&) = default;
  object (object&&) = delete;
  object () : object { value_type { } } { }
  ~object () = default;

  object& operator = (object const&) = default;
  object& operator = (object&&) = delete;

  template <
    class ValueType,
    class=core::enable_if_t<
      not std::is_same<object, core::decay_t<ValueType>>::value
    >
  > object& operator = (ValueType&& value) {
    object { ::std::forward<ValueType>(value) }.swap(*this);
    return *this;
  }

  void swap (object& that) noexcept {
    using std::swap;
    swap(this->handle, that.handle);
  }

  pointer operator -> () const noexcept { return this->handle.get(); }
  operator reference () const noexcept { return this->get(); }

  reference get () const noexcept { return *this->handle; }

private:
  std::shared_ptr<value_type> handle;
};

template <class U, class E, class A, class T>
bool operator == (
  object<U, E, A, T> const& lhs,
  object<U, E, A, T> const& rhs
) noexcept { return std::equal_to<U> { }(lhs, rhs); }

template <class U, class E, class A, class T>
bool operator != (
  object<U, E, A, T> const& lhs,
  object<U, E, A, T> const& rhs
) noexcept { return std::not_equal_to<U> { }(lhs, rhs); }

template <class U, class E, class A, class T>
bool operator >= (
  object<U, E, A, T> const& lhs,
  object<U, E, A, T> const& rhs
) noexcept { return std::greater_equal<U> { }(lhs, rhs); }

template <class U, class E, class A, class T>
bool operator <= (
  object<U, E, A, T> const& lhs,
  object<U, E, A, T> const& rhs
) noexcept { return std::less_equal<U> { }(lhs, rhs); }

template <class U, class E, class A, class T>
bool operator > (
  object<U, E, A, T> const& lhs,
  object<U, E, A, T> const& rhs
) noexcept { return std::greater<U> { }(lhs, rhs); }

template <class U, class E, class A, class T>
bool operator < (
  object<U, E, A, T> const& lhs,
  object<U, E, A, T> const& rhs
) noexcept { return std::less<U> { }(lhs, rhs); }

template <class U, class E, class A, class T>
bool operator == (object<U, E, A, T> const& lhs, U const& rhs) {
  return std::equal_to<U> { }(lhs, rhs);
}

template <class U, class E, class A, class T>
bool operator != (object<U, E, A, T> const& lhs, U const& rhs) {
  return std::not_equal_to<U> { }(lhs, rhs);
}

template <class U, class E, class A, class T>
bool operator >= (object<U, E, A, T> const& lhs, U const& rhs) {
  return std::greater_equal<U> { }(lhs, rhs);
}

template <class U, class E, class A, class T>
bool operator <= (object<U, E, A, T> const& lhs, U const& rhs) {
  return std::less_equal<U> { }(lhs, rhs);
}

template <class U, class E, class A, class T>
bool operator > (object<U, E, A, T> const& lhs, U const& rhs) {
  return std::greater<U> { }(lhs, rhs);
}

template <class U, class E, class A, class T>
bool operator < (object<U, E, A, T> const& lhs, U const& rhs) {
  return std::less<U> { }(lhs, rhs);
}

template <class U, class E, class A, class T>
void swap (object<U, E, A, T>& lhs, object<U, E, A, T>& rhs) noexcept(
  noexcept(lhs.swap(rhs))
) { return lhs.swap(rhs); }

}} /* namespace flyweight::v1 */

namespace std {

template <class U, class E, class A, class T>
struct hash<flyweight::v1::object<U, E, A, T>> {
  using argument_type = flyweight::v1::object<U, E, A, T>;
  using result_type = typename hash<
    typename argument_type::value_type
  >::result_type;

  result_type operator ()(argument_type const& value) const noexcept {
    return hash<typename argument_type::value_type> { }(value);
  }
};

} /* namespace std */

#endif /* FLYWEIGHT_OBJECT_HPP */
