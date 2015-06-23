#ifndef CORE_ANY_HPP
#define CORE_ANY_HPP

#ifdef CORE_NO_RTTI
  #error "core::any requires RTTI"
#endif /* CORE_NO_RTTI */

#include <typeinfo>
#include <memory>

#include <cstdlib>
#include <cstring>

#include <core/type_traits.hpp>
#include <core/utility.hpp>

#ifndef CORE_NO_EXCEPTIONS
#include <stdexcept>
#endif /* CORE_NO_EXCEPTIONS */

namespace core {
inline namespace v1 {
namespace impl {

using data_type = add_pointer_t<void>;

template <class Type>
using is_small = meta::boolean<
  sizeof(decay_t<Type>) <= sizeof(data_type) and
  ::std::is_nothrow_copy_constructible<Type>::value
>;

struct any_dispatch {
  using destroy_function = add_pointer_t<void(data_type&)>;
  using clone_function = add_pointer_t<void(data_type const&, data_type&)>;
  using move_function = add_pointer_t<void(data_type&, data_type&)>;
  using type_function = add_pointer_t<::std::type_info const&()>;

  destroy_function const destroy;
  clone_function const clone;
  move_function const move;
  type_function const type;
};

template <class Type, bool=is_small<Type>::value> struct any_dispatch_select;

template <class Type>
struct any_dispatch_select<Type, true> {
  using value_type = Type;
  using const_pointer = add_pointer_t<add_const_t<value_type>>;
  using pointer = add_pointer_t<value_type>;
  using allocator_type = ::std::allocator<value_type>;
  using allocator_traits = ::std::allocator_traits<allocator_type>;

  static void clone (data_type const& source, data_type& data) {
    allocator_type alloc { };
    auto val = reinterpret_cast<const_pointer const>(::std::addressof(source));
    auto ptr = reinterpret_cast<pointer>(::std::addressof(data));
    allocator_traits::construct(alloc, ptr, *val);
  }

  static void move (data_type& source, data_type& data) {
    allocator_type alloc { };
    auto val = reinterpret_cast<pointer>(::std::addressof(source));
    auto ptr = reinterpret_cast<pointer>(::std::addressof(data));
    allocator_traits::construct(alloc, ptr, ::core::move(*val));
  }

  static void destroy (data_type& data) {
    allocator_type alloc { };
    auto ptr = reinterpret_cast<pointer>(::std::addressof(data));
    allocator_traits::destroy(alloc, ptr);
  }
};

template <class Type>
struct any_dispatch_select<Type, false> {
  using value_type = Type;
  using pointer = add_pointer_t<value_type>;
  using allocator_type = ::std::allocator<value_type>;
  using allocator_traits = ::std::allocator_traits<allocator_type>;

  static void clone (data_type const& source, data_type& data) {
    allocator_type alloc { };
    auto const& value = *static_cast<pointer const>(source);
    auto pointer = allocator_traits::allocate(alloc, 1);
    auto scope = make_scope_guard([&alloc, pointer] {
      allocator_traits::deallocate(alloc, pointer, 1);
    });
    allocator_traits::construct(alloc, pointer, value);
    scope.dismiss();
    data = pointer;
  }

  static void move (data_type& source, data_type& data) {
    allocator_type alloc { };
    auto& value = *static_cast<pointer>(source);
    auto pointer = allocator_traits::allocate(alloc, 1);
    auto scope = make_scope_guard([&alloc, pointer] {
      allocator_traits::deallocate(alloc, pointer, 1);
    });
    allocator_traits::construct(alloc, pointer, ::core::move(value));
    scope.dismiss();
    data = pointer;
  }

  static void destroy (data_type& data) {
    allocator_type alloc { };
    auto value = static_cast<pointer>(data);
    allocator_traits::destroy(alloc, value);
    allocator_traits::deallocate(alloc, value, 1);
  }
};

template <class Type>
any_dispatch const* get_any_dispatch () {
  static any_dispatch const instance = {
    any_dispatch_select<Type>::destroy,
    any_dispatch_select<Type>::clone,
    any_dispatch_select<Type>::move,
    [] () -> ::std::type_info const& { return typeid(Type); }
  };
  return ::std::addressof(instance);
}

template <>
inline any_dispatch const* get_any_dispatch<void> () {
  static any_dispatch const instance = {
    [] (data_type&) { },
    [] (data_type const&, data_type&) { },
    [] (data_type&, data_type&) { },
    [] () -> ::std::type_info const& { return typeid(void); }
  };
  return ::std::addressof(instance);
}

} /* namespace impl */

/* Forgive the ugly preprocessor macro in the middle of this file */
#ifndef CORE_NO_EXCEPTIONS
class bad_any_cast final : public ::std::bad_cast {
public:
  virtual char const* what () const noexcept override {
    return "bad any cast";
  }
};

[[noreturn]] inline void throw_bad_any_cast () { throw bad_any_cast { }; }
#else /* CORE_NO_EXCEPTIONS */
[[noreturn]] inline void throw_bad_any_cast () { ::std::abort(); }
#endif /* CORE_NO_EXCEPTIONS */

class any final {
  template <class ValueType>
  friend ValueType const* any_cast (any const*) noexcept;
  template <class ValueType> friend ValueType* any_cast (any*) noexcept;

  impl::any_dispatch const* table;
  impl::data_type data;

  template <class ValueType>
  any (ValueType&& value, ::std::true_type&&) :
    table { impl::get_any_dispatch<decay_t<ValueType>>() },
    data { nullptr }
  {
    using value_type = decay_t<ValueType>;
    using allocator_type = ::std::allocator<value_type>;
    allocator_type alloc { };
    auto pointer = reinterpret_cast<value_type*>(::std::addressof(this->data));
    ::std::allocator_traits<allocator_type>::construct(
      alloc, pointer, ::std::forward<ValueType>(value)
    );
  }

  template <class ValueType>
  any (ValueType&& value, ::std::false_type&&) :
    table { impl::get_any_dispatch<decay_t<ValueType>>() },
    data { nullptr }
  {
    using value_type = decay_t<ValueType>;
    using allocator_type = ::std::allocator<value_type>;
    allocator_type alloc { };
    auto pointer = ::std::allocator_traits<allocator_type>::allocate(alloc, 1);
    ::std::allocator_traits<allocator_type>::construct(
      alloc, pointer, ::std::forward<ValueType>(value)
    );
    this->data = pointer;
  }

  template <class ValueType>
  ValueType const* cast (::std::true_type&&) const {
    return reinterpret_cast<ValueType const*>(::std::addressof(this->data));
  }

  template <class ValueType>
  ValueType* cast (::std::true_type&&) {
    return reinterpret_cast<ValueType*>(::std::addressof(this->data));
  }

  template <class ValueType>
  ValueType const* cast (::std::false_type&&) const {
    return static_cast<ValueType const*>(this->data);
  }

  template <class ValueType>
  ValueType* cast (::std::false_type&&) {
    return static_cast<ValueType*>(this->data);
  }

public:
  any (any const& that) :
    table { that.table },
    data { nullptr }
  { this->table->clone(that.data, this->data); }

  any (any&& that) noexcept :
    table { that.table },
    data { nullptr }
  { this->table->move(that.data, this->data); }

  any () noexcept :
    table { impl::get_any_dispatch<void>() },
    data { nullptr }
  { }

  template <
    class ValueType,
    class=enable_if_t<not ::std::is_same<any, decay_t<ValueType>>::value>
  > any (ValueType&& value) :
    any { ::std::forward<ValueType>(value), impl::is_small<ValueType> { } }
  { }

  ~any () noexcept { this->clear(); }

  any& operator = (any const& that) {
    any { that }.swap(*this);
    return *this;
  }

  any& operator = (any&& that) noexcept {
    any { ::std::move(that) }.swap(*this);
    return *this;
  }

  template <
    class ValueType,
    class=enable_if_t<not ::std::is_same<any, decay_t<ValueType>>::value>
  > any& operator = (ValueType&& value) {
    any {
      ::std::forward<ValueType>(value),
      impl::is_small<ValueType> { }
    }.swap(*this);
    return *this;
  }

  void swap (any& that) noexcept {
    using ::std::swap;
    swap(this->table, that.table);
    swap(this->data, that.data);
  }

  void clear () noexcept {
    this->table->destroy(this->data);
    this->table = impl::get_any_dispatch<void>();
  }

  ::std::type_info const& type () const noexcept {
    return this->table->type();
  }

  bool empty () const noexcept {
    return this->table == impl::get_any_dispatch<void>();
  }

};

template <class ValueType>
ValueType const* any_cast (any const* operand) noexcept {
  return operand and operand->type() == typeid(ValueType)
    ? operand->cast<ValueType>(impl::is_small<ValueType> { })
    : nullptr;
}

template <class ValueType>
ValueType* any_cast (any* operand) noexcept {
  return operand and operand->type() == typeid(ValueType)
    ? operand->cast<ValueType>(impl::is_small<ValueType> { })
    : nullptr;
}

template <
  class ValueType,
  class=enable_if_t<
    meta::any<
      ::std::is_reference<ValueType>,
      ::std::is_copy_constructible<ValueType>
    >::value
  >
> ValueType any_cast (any const& operand) {
  using type = remove_reference_t<ValueType>;
  auto pointer = any_cast<add_const_t<type>>(::std::addressof(operand));
  if (not pointer) { throw_bad_any_cast(); }
  return *pointer;
}

template <
  class ValueType,
  class=enable_if_t<
    meta::any<
      ::std::is_reference<ValueType>,
      ::std::is_copy_constructible<ValueType>
    >::value
  >
> ValueType any_cast (any&& operand) {
  using type = remove_reference_t<ValueType>;
  auto pointer = any_cast<type>(::std::addressof(operand));
  if (not pointer) { throw_bad_any_cast(); }
  return *pointer;
}

template <
  class ValueType,
  class=enable_if_t<
    meta::any<
      ::std::is_reference<ValueType>,
      ::std::is_copy_constructible<ValueType>
    >::value
  >
> ValueType any_cast (any& operand) {
  using type = remove_reference_t<ValueType>;
  auto pointer = any_cast<type>(::std::addressof(operand));
  if (not pointer) { throw_bad_any_cast(); }
  return *pointer;
}

inline void swap (any& lhs, any& rhs) noexcept { lhs.swap(rhs); }

}} /* namespace core::v1 */

#endif /* CORE_ANY_HPP */
