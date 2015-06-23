#ifndef CORE_MEMORY_HPP
#define CORE_MEMORY_HPP

#include <memory>
#include <bitset>
#include <tuple>

#include <cstddef>
#include <cstdlib>

#include <core/type_traits.hpp>
#include <core/algorithm.hpp>
#include <core/range.hpp>

#ifndef CORE_NO_EXCEPTIONS
#include <stdexcept>
#endif /* CORE_NO_EXCEPTIONS */

#ifndef CORE_NO_RTTI
#include <typeinfo>
#endif /* CORE_NO_RTTI */

/* Small hack just for GCC 4.8. Just trust me, it's needed */
#if defined(__GNUC__) && defined(__GNUC_MINOR__)
  #if __GNUC__ == 4 && __GNUC_MINOR__ == 8
  namespace std { using ::max_align_t; } /* namespace std */
  #endif
#endif /* defined(__GNUC__) */

namespace core {
inline namespace v1 {
namespace impl {

template <class T>
using deep_lvalue = conditional_t<
  ::std::is_reference<T>::value, T, T const&
>;

template <class T, class D, class=void> struct pointer :
  identity<add_pointer_t<T>>
{ };

template <class T, class D>
struct pointer<T, D, deduce_t<typename D::pointer>> :
  identity<typename D::pointer>
{ };

template <class T, class D> using pointer_t = typename pointer<T, D>::type;

} /* namespace impl */

namespace memory {

template <::std::size_t N>
struct arena final {
  static_assert(N != 0, "N may not be 0");

  using size_type = ::std::size_t;

  arena () noexcept : current { } { }
  ~arena () noexcept = default;

  arena (arena const&) noexcept = delete;
  arena& operator = (arena const&) noexcept = delete;

  static constexpr size_type alignment () noexcept {
    return alignof(::std::max_align_t);
  }
  static constexpr size_type size () noexcept { return N; }
  size_type max_size () const noexcept { return N; }
  size_type used () const noexcept { return this->current; }
  void reset () noexcept { this->current = 0; }

  void* allocate (size_type n) {
    if (not this->inside(n)) { return nullptr; }
    auto ptr = this->pointer() + this->current;
    this->current += n;
    return ptr;
  }

  void deallocate (void* p, size_type n) {
    auto incoming = static_cast<::std::uint8_t*>(p) + n;
    auto ptr = this->pointer() + this->current;
    if (ptr != incoming) { return; }
    this->current -= n;
  }

  /* used by arena_allocator to permit default construction */
  static arena& ref () noexcept {
    static arena instance;
    return instance;
  }

private:
  bool inside (size_type n) const noexcept { return (this->current + n) < N; }
  ::std::uint8_t* pointer () noexcept {
    return reinterpret_cast<::std::uint8_t*>(::std::addressof(this->data));
  }

  aligned_storage_t<N, alignof(::std::max_align_t)> data;
  size_type current; // byte index
};

} /* namespace memory */

template <class T, ::std::size_t N>
struct arena_allocator {
  using difference_type = ::std::ptrdiff_t;
  using value_type = T;
  using size_type = ::std::size_t;

  using const_reference = add_lvalue_reference_t<add_const_t<value_type>>;
  using const_pointer = add_pointer_t<add_const_t<value_type>>;
  using reference = add_lvalue_reference_t<value_type>;
  using pointer = add_pointer_t<value_type>;

  using const_void_pointer = add_pointer_t<add_const_t<void>>;
  using void_pointer = add_pointer_t<void>;

  using propagate_on_container_copy_assignment = ::std::true_type;
  using propagate_on_container_move_assignment = ::std::true_type;
  using propagate_on_container_swap = ::std::true_type;

  using is_always_equal = ::std::false_type;

  template <class, ::std::size_t> friend struct arena_allocator;
  template <class U> struct rebind { using other = arena_allocator<U, N>; };

  explicit arena_allocator (memory::arena<N>& ref) noexcept : ref { ref } { }
  arena_allocator () noexcept : arena_allocator { memory::arena<N>::ref() } { }

  template <class U>
  arena_allocator (arena_allocator<U, N> const& that) noexcept :
    ref { that.ref }
  { }

  arena_allocator (arena_allocator const& that) noexcept :
    ref { that.ref }
  { }

  template <class U>
  arena_allocator& operator = (arena_allocator<U, N> const& that) noexcept {
    arena_allocator { that }.swap(*this);
    return *this;
  }

  arena_allocator& operator = (arena_allocator const& that) noexcept = default;

  void swap (arena_allocator& that) noexcept {
    ::std::swap(this->ref, that.ref);
  }

  pointer allocate (size_type n, const_void_pointer=nullptr) noexcept {
    return static_cast<pointer>(this->arena().allocate(n * sizeof(value_type)));
  }

  void deallocate (pointer ptr, size_type n) noexcept {
    this->arena().deallocate(ptr, n * sizeof(value_type));
  }

  size_type max_size () const noexcept { return N / sizeof(value_type); }

  memory::arena<N> const& arena () const noexcept { return this->ref; }
  memory::arena<N>& arena () noexcept { return this->ref; }

private:
  ::std::reference_wrapper<memory::arena<N>> ref;
};

/* poly-ptr related definitions */
#ifndef CORE_NO_RTTI
/* poly_ptr copier */
template <class T, class D, class U>
::std::unique_ptr<T, D> default_poly_copy (
  ::std::unique_ptr<T, D> const& ptr
) {
  auto value = *dynamic_cast<U*>(ptr.get());
  auto const& deleter = ptr.get_deleter();
  return ::std::unique_ptr<T, D> { new U { ::std::move(value) }, deleter };
}

/* null-state poly_ptr copier (don't copy that poly!) */
template <class T, class D>
::std::unique_ptr<T, D> null_poly_copy (
  ::std::unique_ptr<T, D> const&
) noexcept { return ::std::unique_ptr<T, D> { }; }
#endif /* CORE_NO_RTTI */

#ifndef CORE_NO_EXCEPTIONS
struct bad_polymorphic_reset : ::std::logic_error {
  using ::std::logic_error::logic_error;
};

[[noreturn]] inline void throw_bad_poly_reset (char const* msg) {
  throw bad_polymorphic_reset { msg };
}
#else /* CORE_NO_EXCEPTIONS */
[[noreturn]] inline void throw_bad_poly_reset (char const*) { ::std::abort(); }
#endif /* CORE_NO_EXCEPTIONS */

/* deep_ptr copier */
template <class T>
struct default_copy {
  using pointer = T*;

  constexpr default_copy () = default;
  template <class U> default_copy (default_copy<U> const&) noexcept { }

  pointer operator ()(pointer const ptr) const { return new T { *ptr }; }
};

#ifndef CORE_NO_RTTI
template <class T, class Deleter=::std::default_delete<T>>
struct poly_ptr final {
  using unique_type = ::std::unique_ptr<T, Deleter>;
  using element_type = typename unique_type::element_type;
  using deleter_type = typename unique_type::deleter_type;
  using copier_type = unique_type (*)(unique_type const&);
  using pointer = typename unique_type::pointer;

  template <class U>
  explicit poly_ptr (U* ptr) noexcept :
    poly_ptr { ptr, deleter_type { } }
  { }

  template <class U, class E>
  explicit poly_ptr (
    ::std::unique_ptr<U, E>&& ptr,
    copier_type copier=::std::addressof(
      default_poly_copy<element_type, deleter_type, U>
    )
  ) noexcept :
    copier { copier },
    ptr { ::std::move(ptr) }
  {
    constexpr bool abstract = ::std::is_abstract<U>::value;
    constexpr bool base = ::std::is_base_of<element_type, U>::value;

    static_assert(not abstract, "cannot create poly_ptr with abstract ptr");
    static_assert(base, "cannot create poly_ptr with non-derived type");
  }

  template <class U, class E>
  poly_ptr (
    U* ptr, E&& deleter,
    copier_type copier=::std::addressof(
      default_poly_copy<element_type, deleter_type, U>
    )
  ) noexcept :
    poly_ptr {
      unique_type { ::std::move(ptr), ::std::forward<E>(deleter) }, copier
    }
  { }

  poly_ptr (poly_ptr const& that) :
    copier { that.copier },
    ptr { that.copier(that.ptr) }
  { }

  poly_ptr (poly_ptr&& that) noexcept :
    copier { ::std::move(that.copier) },
    ptr { ::std::move(that.ptr) }
  { that.copier = null_poly_copy<element_type, deleter_type>; }

  constexpr poly_ptr () noexcept { }

  ~poly_ptr () noexcept { }

  template <class U, class E>
  poly_ptr& operator = (::std::unique_ptr<U, E>&& ptr) {
    poly_ptr { ::std::move(ptr) }.swap(*this);
    return *this;
  }

  template <class U>
  poly_ptr& operator = (U* ptr) {
    poly_ptr { ptr }.swap(*this);
    return *this;
  }

  poly_ptr& operator = (::std::nullptr_t) noexcept {
    this->reset();
    return *this;
  }

  poly_ptr& operator = (poly_ptr const& that) {
    return *this = poly_ptr { that };
  }

  poly_ptr& operator = (poly_ptr&& that) noexcept {
    poly_ptr { ::std::move(that) }.swap(*this);
    return *this;
  }

  explicit operator bool () const noexcept { return bool(this->ptr); }

  add_lvalue_reference_t<element_type> operator * () const noexcept {
    return *this->ptr;
  }

  pointer operator -> () const noexcept { return this->ptr.get(); }

  pointer get () const noexcept { return this->ptr.get(); }

  deleter_type const& get_deleter () const noexcept {
    return this->ptr.get_deleter();
  }

  deleter_type& get_deleter () noexcept { return this->ptr.get_deleter(); }

  copier_type const& get_copier () const noexcept { return this->copier; }
  copier_type& get_copier () noexcept { return this->copier; }

  pointer release () noexcept {
    this->copier = null_poly_copy<element_type, deleter_type>;
    return this->ptr.release();
  }

  void reset (pointer ptr = pointer { }) {
    constexpr auto invalid = "cannot reset null poly_ptr with valid pointer";
    constexpr auto type = "cannot reset poly_ptr with different type";

    if (ptr and not this->ptr) { throw_bad_poly_reset(invalid); }
    if (ptr and typeid(*this->ptr) != typeid(*ptr)) {
      throw_bad_poly_reset(type);
    }

    this->ptr.reset(ptr);
    if (not ptr) { this->copier = null_poly_copy<element_type, deleter_type>; }
  }

  void swap (poly_ptr& that) noexcept {
    using ::std::swap;
    swap(this->get_copier(), that.get_copier());
    swap(this->ptr, that.ptr);
  }

private:
  static_assert(
    ::std::is_polymorphic<element_type>::value,
    "cannot create a poly_ptr with a non-polymorphic type"
  );

  copier_type copier { null_poly_copy<element_type, deleter_type> };
  unique_type ptr;
};
#endif /* CORE_NO_RTTI */

template <
  class T,
  class Deleter=::std::default_delete<T>,
  class Copier=default_copy<T>
> struct deep_ptr final {

  using element_type = T;
  using deleter_type = Deleter;
  using copier_type = Copier;
  using pointer = impl::pointer_t<element_type, deleter_type>;

  static_assert(
    ::std::is_same<result_of_t<copier_type(pointer)>, pointer>::value,
    "deleter_type and copier_type have differing pointer types"
  );

  using data_type = ::std::tuple<pointer, deleter_type, copier_type>;

  deep_ptr (
    pointer ptr,
    impl::deep_lvalue<deleter_type> deleter,
    impl::deep_lvalue<copier_type> copier
  ) noexcept :
    data { ptr, deleter, copier }
  { }

  deep_ptr (
    pointer ptr,
    remove_reference_t<deleter_type>&& deleter,
    remove_reference_t<copier_type>&& copier
  ) noexcept :
    data { ::std::move(ptr), ::std::move(deleter), ::std::move(copier) }
  { }

  template <class U, class E>
  deep_ptr (::std::unique_ptr<U, E>&& that) noexcept :
    deep_ptr {
      that.release(),
      ::std::move(that.get_deleter()),
      copier_type { }
    }
  { }

  explicit deep_ptr (pointer ptr) noexcept :
    deep_ptr { ptr, deleter_type { }, copier_type { } }
  { }

  constexpr deep_ptr (::std::nullptr_t) noexcept : deep_ptr { } { }

  deep_ptr (deep_ptr const& that) :
    deep_ptr {
      that.get() ? that.get_copier()(that.get()) : that.get(),
      that.get_deleter(),
      that.get_copier()
    }
  { }

  deep_ptr (deep_ptr&& that) noexcept :
    data {
      that.release(),
      ::std::move(that.get_deleter()),
      ::std::move(that.get_copier())
    }
  { }

  constexpr deep_ptr () noexcept : data { } { }

  ~deep_ptr () noexcept {
    auto& ptr = ::std::get<0>(this->data);
    if (not ptr) { return; }
    this->get_deleter()(ptr);
    ptr = nullptr;
  }

  deep_ptr& operator = (::std::nullptr_t) noexcept {
    this->reset();
    return *this;
  }

  deep_ptr& operator = (deep_ptr const& that) {
    return *this = deep_ptr { that };
  }

  deep_ptr& operator = (deep_ptr&& that) noexcept {
    deep_ptr { ::std::move(that) }.swap(*this);
    return *this;
  }

  explicit operator bool () const noexcept { return this->get(); }

  add_lvalue_reference_t<element_type> operator * () const noexcept {
    return *this->get();
  }
  pointer operator -> () const noexcept { return this->get(); }
  pointer get () const noexcept { return ::std::get<0>(this->data); }

  deleter_type const& get_deleter () const noexcept {
    return ::std::get<1>(this->data);
  }

  deleter_type& get_deleter () noexcept { return ::std::get<1>(this->data); }

  copier_type const& get_copier () const noexcept {
    return ::std::get<2>(this->data);
  }

  copier_type& get_copier () noexcept { return ::std::get<2>(this->data); }

  pointer release () noexcept {
    auto ptr = this->get();
    ::std::get<0>(this->data) = nullptr;
    return ptr;
  }

  void reset (pointer ptr = pointer { }) noexcept {
    using ::std::swap;
    swap(::std::get<0>(this->data), ptr);
    if (not ptr) { return; }
    this->get_deleter()(ptr);
  }

  void swap (deep_ptr& that) noexcept(is_nothrow_swappable<data_type>::value) {
    using ::std::swap;
    swap(this->data, that.data);
  }

private:
  data_type data;
};

template <class W>
struct observer_ptr final {
  using element_type = W;

  using const_pointer = add_pointer_t<add_const_t<element_type>>;
  using pointer = add_pointer_t<element_type>;

  using const_reference = add_lvalue_reference_t<add_const_t<element_type>>;
  using reference = add_lvalue_reference_t<element_type>;

  observer_ptr (observer_ptr const&) noexcept = default;

  constexpr observer_ptr (::std::nullptr_t) noexcept : ptr { nullptr } { }
  explicit observer_ptr (pointer ptr) noexcept : ptr { ptr } { }

  template <
    class T,
    class=enable_if_t<::std::is_convertible<pointer, add_pointer_t<T>>::value>
  > explicit observer_ptr (add_pointer_t<T> ptr) noexcept :
    ptr { dynamic_cast<pointer>(ptr) }
  { }

  template <
    class T,
    class=enable_if_t<::std::is_convertible<pointer, add_pointer_t<T>>::value>
  > observer_ptr (observer_ptr<T> const& that) noexcept :
    observer_ptr { that.get() }
  { }

  constexpr observer_ptr () noexcept : observer_ptr { nullptr } { }
  ~observer_ptr () noexcept { this->ptr = nullptr; }

  template <
    class T,
    class=enable_if_t<::std::is_convertible<pointer, add_pointer_t<T>>::value>
  > observer_ptr& operator = (add_pointer_t<T> ptr) noexcept {
    observer_ptr { ptr }.swap(*this);
    return *this;
  }

  observer_ptr& operator = (observer_ptr const&) noexcept = default;

  template <
    class T,
    class=enable_if_t<::std::is_convertible<pointer, add_pointer_t<T>>::value>
  > observer_ptr& operator = (observer_ptr<T> const& that) noexcept {
    observer_ptr { that }.swap(*this);
    return *this;
  }

  observer_ptr& operator = (::std::nullptr_t) noexcept {
    this->reset();
    return *this;
  }

  void swap (observer_ptr& that) noexcept {
    using ::std::swap;
    swap(this->ptr, that.ptr);
  }

  explicit operator const_pointer () const noexcept { return this->get(); }
  explicit operator pointer () noexcept { return this->get(); }
  explicit operator bool () const noexcept { return this->get(); }

  reference operator * () const noexcept { return *this->get(); }
  pointer operator -> () const noexcept { return this->get(); }
  pointer get () const noexcept { return this->ptr; }

  pointer release () noexcept {
    auto result = this->get();
    this->reset();
    return result;
  }

  void reset (pointer ptr = nullptr) noexcept { this->ptr = ptr; }

private:
  pointer ptr;
};

template <class T, ::std::size_t N, class U, ::std::size_t M>
bool operator == (
  arena_allocator<T, N> const& lhs,
  arena_allocator<U, M> const& rhs
) noexcept {
  return N == M and
    ::std::addressof(lhs.arena()) == ::std::addressof(rhs.arena());
}

template <class T, ::std::size_t N, class U, ::std::size_t M>
bool operator != (
  arena_allocator<T, N> const& lhs,
  arena_allocator<U, M> const& rhs
) noexcept { return N != M or
  ::std::addressof(lhs.arena()) != ::std::addressof(rhs.arena());
}

#ifndef CORE_NO_RTTI
/* poly_ptr convention for type and deleter is: T, D : U, E */
template <class T, class D, class U, class E>
bool operator == (
  poly_ptr<T, D> const& lhs,
  poly_ptr<U, E> const& rhs
) noexcept { return lhs.get() == rhs.get(); }

template <class T, class D, class U, class E>
bool operator != (
  poly_ptr<T, D> const& lhs,
  poly_ptr<U, E> const& rhs
) noexcept { return lhs.get() != rhs.get(); }

template <class T, class D, class U, class E>
bool operator >= (
  poly_ptr<T, D> const& lhs,
  poly_ptr<U, E> const& rhs
) noexcept { return not (lhs < rhs); }

template <class T, class D, class U, class E>
bool operator <= (
  poly_ptr<T, D> const& lhs,
  poly_ptr<U, E> const& rhs
) noexcept { return not (rhs < lhs); }

template <class T, class D, class U, class E>
bool operator > (
  poly_ptr<T, D> const& lhs,
  poly_ptr<U, E> const& rhs
) noexcept { return rhs < lhs; }

template <class T, class D, class U, class E>
bool operator < (
  poly_ptr<T, D> const& lhs,
  poly_ptr<U, E> const& rhs
) noexcept {
  using common_type = typename ::std::common_type<
    typename poly_ptr<T, D>::pointer,
    typename poly_ptr<U, E>::pointer
  >::type;
  return ::std::less<common_type> { }(lhs.get(), rhs.get());
}
#endif /* CORE_NO_RTTI */

/* deep_ptr convention for type, deleter, copier is
 * T, D, C : U, E, K
 */
template <class T, class D, class C, class U, class E, class K>
bool operator == (
  deep_ptr<T, D, C> const& lhs,
  deep_ptr<U, E, K> const& rhs
) noexcept { return lhs.get() == rhs.get(); }

template <class T, class D, class C, class U, class E, class K>
bool operator != (
  deep_ptr<T, D, C> const& lhs,
  deep_ptr<U, E, K> const& rhs
) noexcept { return lhs.get() != rhs.get(); }

template <class T, class D, class C, class U, class E, class K>
bool operator >= (
  deep_ptr<T, D, C> const& lhs,
  deep_ptr<U, E, K> const& rhs
) noexcept { return not (lhs < rhs); }

template <class T, class D, class C, class U, class E, class K>
bool operator <= (
  deep_ptr<T, D, C> const& lhs,
  deep_ptr<U, E, K> const& rhs
) noexcept { return not (rhs < lhs); }

template <class T, class D, class C, class U, class E, class K>
bool operator > (
  deep_ptr<T, D, C> const& lhs,
  deep_ptr<U, E, K> const& rhs
) noexcept { return rhs < lhs; }

template <class T, class D, class C, class U, class E, class K>
bool operator < (
  deep_ptr<T, D, C> const& lhs,
  deep_ptr<U, E, K> const& rhs
) noexcept {
  using common_type = common_type_t<
    typename deep_ptr<T, D, C>::pointer,
    typename deep_ptr<U, E, K>::pointer
  >;
  return ::std::less<common_type> { }(lhs.get(), rhs.get());
}

#ifndef CORE_NO_RTTI
/* poly_ptr nullptr operator overloads */
template <class T, class D>
bool operator == (poly_ptr<T, D> const& lhs, ::std::nullptr_t) noexcept {
  return not lhs;
}

template <class T, class D>
bool operator == (::std::nullptr_t, poly_ptr<T, D> const& rhs) noexcept {
  return not rhs;
}

template <class T, class D>
bool operator != (poly_ptr<T, D> const& lhs, ::std::nullptr_t) noexcept {
  return bool(lhs);
}

template <class T, class D>
bool operator != (::std::nullptr_t, poly_ptr<T, D> const& rhs) noexcept {
  return bool(rhs);
}

template <class T, class D>
bool operator >= (poly_ptr<T, D> const& lhs, ::std::nullptr_t) noexcept {
  return not (lhs < nullptr);
}

template <class T, class D>
bool operator >= (::std::nullptr_t, poly_ptr<T, D> const& rhs) noexcept {
  return not (nullptr < rhs);
}

template <class T, class D>
bool operator <= (poly_ptr<T, D> const& lhs, ::std::nullptr_t) noexcept {
  return not (nullptr < lhs);
}

template <class T, class D>
bool operator <= (::std::nullptr_t, poly_ptr<T, D> const& rhs) noexcept {
  return not (rhs < nullptr);
}

template <class T, class D>
bool operator > (poly_ptr<T, D> const& lhs, ::std::nullptr_t) noexcept {
  return nullptr < lhs;
}

template <class T, class D>
bool operator > (::std::nullptr_t, poly_ptr<T, D> const& rhs) noexcept {
  return rhs < nullptr;
}

template <class T, class D>
bool operator < (poly_ptr<T, D> const& lhs, ::std::nullptr_t) noexcept {
  using pointer = typename poly_ptr<T, D>::pointer;
  return ::std::less<pointer> { }(lhs.get(), nullptr);
}

template <class T, class D>
bool operator < (::std::nullptr_t, poly_ptr<T, D> const& rhs) noexcept {
  using pointer = typename poly_ptr<T, D>::pointer;
  return ::std::less<pointer> { }(nullptr, rhs.get());
}
#endif /* CORE_NO_RTTI */

/* deep_ptr nullptr operator overloads */
template <class T, class D, class C>
bool operator == (deep_ptr<T, D, C> const& lhs, ::std::nullptr_t) noexcept {
  return not lhs;
}

template <class T, class D, class C>
bool operator == (::std::nullptr_t, deep_ptr<T, D, C> const& rhs) noexcept {
  return not rhs;
}

template <class T, class D, class C>
bool operator != (deep_ptr<T, D, C> const& lhs, ::std::nullptr_t) noexcept {
  return bool(lhs);
}

template <class T, class D, class C>
bool operator != (::std::nullptr_t, deep_ptr<T, D, C> const& rhs) noexcept {
  return bool(rhs);
}

template <class T, class D, class C>
bool operator >= (deep_ptr<T, D, C> const& lhs, ::std::nullptr_t) noexcept {
  return not (lhs < nullptr);
}

template <class T, class D, class C>
bool operator >= (::std::nullptr_t, deep_ptr<T, D, C> const& rhs) noexcept {
  return not (nullptr < rhs);
}

template <class T, class D, class C>
bool operator <= (deep_ptr<T, D, C> const& lhs, ::std::nullptr_t) noexcept {
  return not (nullptr < lhs);
}

template <class T, class D, class C>
bool operator <= (::std::nullptr_t, deep_ptr<T, D, C> const& rhs) noexcept {
  return not (rhs < nullptr);
}

template <class T, class D, class C>
bool operator > (deep_ptr<T, D, C> const& lhs, ::std::nullptr_t) noexcept {
  return nullptr < lhs;
}

template <class T, class D, class C>
bool operator > (::std::nullptr_t, deep_ptr<T, D, C> const& rhs) noexcept {
  return rhs < nullptr;
}

template <class T, class D, class C>
bool operator < (deep_ptr<T, D, C> const& lhs, ::std::nullptr_t) noexcept {
  using pointer = typename deep_ptr<T, D, C>::pointer;
  return ::std::less<pointer> { }(lhs.get(), nullptr);
}

template <class T, class D, class C>
bool operator < (::std::nullptr_t, deep_ptr<T, D, C> const& rhs) noexcept {
  using pointer = typename deep_ptr<T, D, C>::pointer;
  return ::std::less<pointer> { }(nullptr, rhs.get());
}

/* observer_ptr and nullptr overloads */
template <class T, class U>
bool operator == (
  observer_ptr<T> const& lhs,
  observer_ptr<U> const& rhs
) noexcept { return lhs.get() == rhs.get(); }

template <class T, class U>
bool operator != (
  observer_ptr<T> const& lhs,
  observer_ptr<U> const& rhs
) noexcept { return lhs.get() != rhs.get(); }

template <class T>
bool operator == (observer_ptr<T> const& lhs, ::std::nullptr_t) noexcept {
  return lhs.get() == nullptr;
}

template <class T>
bool operator != (observer_ptr<T> const& lhs, ::std::nullptr_t) noexcept {
  return lhs.get() != nullptr;
}

template <class T>
bool operator == (::std::nullptr_t, observer_ptr<T> const& rhs) noexcept {
  return nullptr == rhs.get();
}

template <class T>
bool operator != (::std::nullptr_t, observer_ptr<T> const& rhs) noexcept {
  return nullptr != rhs.get();
}

template <class T, class U>
bool operator >= (
  observer_ptr<T> const& lhs,
  observer_ptr<U> const& rhs
) noexcept { return lhs.get() >= rhs.get(); }

template <class T, class U>
bool operator <= (
  observer_ptr<T> const& lhs,
  observer_ptr<U> const& rhs
) noexcept { return lhs.get() <= rhs.get(); }

template <class T, class U>
bool operator > (
  observer_ptr<T> const& lhs,
  observer_ptr<U> const& rhs
) noexcept { return lhs.get() > rhs.get(); }

template <class T, class U>
bool operator < (
  observer_ptr<T> const& lhs,
  observer_ptr<U> const& rhs
) noexcept { return lhs.get() < rhs.get(); }

/* make_observer */
template <class W>
observer_ptr<W> make_observer (W* ptr) noexcept {
  return observer_ptr<W> { ptr };
}

template <class W, class D>
observer_ptr<W> make_observer (::std::unique_ptr<W, D> const& ptr) noexcept {
  return observer_ptr<W> { ptr.get() };
}

template <class W>
observer_ptr<W> make_observer (::std::shared_ptr<W> const& ptr) noexcept {
  return observer_ptr<W> { ptr.get() };
}

template <class W>
observer_ptr<W> make_observer (::std::weak_ptr<W> const& ptr) noexcept {
  return make_observer(ptr.lock());
}

template <class W, class C, class D>
observer_ptr<W> make_observer (deep_ptr<W, C, D> const& ptr) noexcept {
  return observer_ptr<W> { ptr.get() };
}

#ifndef CORE_NO_RTTI
template <class W, class D>
observer_ptr<W> make_observer (poly_ptr<W, D> const& ptr) noexcept {
  return observer_ptr<W> { ptr.get() };
}
#endif /* CORE_NO_RTTI */

#ifndef CORE_NO_RTTI
/* make_poly */
template <
  class T,
  class U,
  class=enable_if_t<
    ::std::is_polymorphic<T>::value and ::std::is_base_of<T, U>::value
  >
> auto make_poly (U&& value) -> poly_ptr<T> {
  return poly_ptr<T> { new U { ::std::forward<U>(value) } };
}
#endif /* CORE_NO_RTTI */

/* make_deep */
template <
  class T,
  class=enable_if_t<not ::std::is_array<T>::value>,
  class... Args
> auto make_deep (Args&&... args) -> deep_ptr<T> {
  return deep_ptr<T> { new T { ::std::forward<Args>(args)... } };
}

/* make_unique */
template <
  class Type,
  class=enable_if_t<not ::std::is_array<Type>::value>,
  class... Args
> auto make_unique(Args&&... args) -> ::std::unique_ptr<Type> {
  return ::std::unique_ptr<Type> {
    new Type { ::std::forward<Args>(args)... }
  };
}

template <
  class Type,
  class=enable_if_t< ::std::is_array<Type>::value>,
  class=enable_if_t<not ::std::extent<Type>::value>
> auto make_unique(::std::size_t size) -> ::std::unique_ptr<Type> {
  return ::std::unique_ptr<Type> { new remove_extent_t<Type>[size] { } };
}

template <
  class Type,
  class=enable_if_t< ::std::is_array<Type>::value>,
  class=enable_if_t< ::std::extent<Type>::value>,
  class... Args
> auto make_unique(Args&&...) -> void = delete;

template <class T, ::std::size_t N>
void swap (arena_allocator<T, N>& lhs, arena_allocator<T, N>& rhs) noexcept {
  lhs.swap(rhs);
}

#ifndef CORE_NO_RTTI
template <class T, class D>
void swap (poly_ptr<T, D>& lhs, poly_ptr<T, D>& rhs) noexcept(
  noexcept(lhs.swap(rhs))
) { lhs.swap(rhs); }
#endif /* CORE_NO_RTTI */

template <class T, class D, class C>
void swap (deep_ptr<T, D, C>& lhs, deep_ptr<T, D, C>& rhs) noexcept(
  noexcept(lhs.swap(rhs))
) { lhs.swap(rhs); }

template <class W>
void swap (observer_ptr<W>& lhs, observer_ptr<W>& rhs) noexcept(
  noexcept(lhs.swap(rhs))
) { lhs.swap(rhs); }

}} /* namespace core::v1 */

namespace std {

#ifndef CORE_NO_RTTI
template <class T, class D>
struct hash<core::v1::poly_ptr<T, D>> {
  using value_type = core::v1::poly_ptr<T, D>;
  size_t operator ()(value_type const& value) const noexcept {
    return hash<typename value_type::pointer>{ }(value.get());
  }
};
#endif /* CORE_NO_RTTI */

template <class T, class Deleter, class Copier>
struct hash<core::v1::deep_ptr<T, Deleter, Copier>> {
  using value_type = core::v1::deep_ptr<T, Deleter, Copier>;
  size_t operator ()(value_type const& value) const noexcept {
    return hash<typename value_type::pointer> { }(value.get());
  }
};

template <class W>
struct hash<core::v1::observer_ptr<W>> {
  using value_type = core::v1::observer_ptr<W>;
  size_t operator ()(value_type const& value) const noexcept {
    return hash<typename value_type::pointer> { }(value.get());
  }
};

} /* namespace std */

#endif /* CORE_MEMORY_HPP */
