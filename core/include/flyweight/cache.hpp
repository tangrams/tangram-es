#ifndef FLYWEIGHT_CACHE_HPP
#define FLYWEIGHT_CACHE_HPP

#include <unordered_map>
#include <algorithm>
#include <memory>
#include <mutex>

#include <flyweight/extractor.hpp>
#include <core/type_traits.hpp>

namespace flyweight {
inline namespace v1 {
namespace impl {

template <class T>
using const_ref = core::add_lvalue_reference_t<core::add_const_t<T>>;

template <class T, class KeyExtractor, class Allocator>
struct container_traits final {

  using allocator_type = typename std::allocator_traits<
    Allocator
  >::template rebind_alloc<T>;

  using extractor_type = KeyExtractor;
  using computed_key_type = core::decay_t<
    decltype(
      std::declval<const_ref<extractor_type>>()(std::declval<const_ref<T>>())
    )
  >;

  using is_associative = std::integral_constant<
    bool,
    not std::is_same<
      core::decay_t<T>,
      core::decay_t<computed_key_type>
    >::value
  >;

  using mapped_type = std::weak_ptr<core::add_const_t<T>>;
  using key_type = core::conditional_t<
    sizeof(computed_key_type) <=
    sizeof(std::reference_wrapper<computed_key_type>) or
    not is_associative::value,
    computed_key_type,
    std::reference_wrapper<core::add_const_t<computed_key_type>>
  >;

  using container_type = std::unordered_map<
    key_type,
    mapped_type,
    std::hash<computed_key_type>,
    std::equal_to<computed_key_type>,
    typename std::allocator_traits<Allocator>::template rebind_alloc<
      std::pair<core::add_const_t<key_type>, mapped_type>
    >
  >;
};

} /* namespace impl */

template <class... Ts> struct tag final { };

/* The cache acts as the backing store for the flyweight, and is partially
 * inspired by the 'thread-safe reference counted object cache' suggested
 * by Herb Sutter.
 */
template <
  class T,
  class KeyExtractor=extractor<T>,
  class Allocator=std::allocator<T>,
  class Tag=tag<>
> struct cache final {

  using traits = impl::container_traits<T, KeyExtractor, Allocator>;

  /* All of these lining up is entirely a coincidence, I assure you */
  using container_type = typename traits::container_type;
  using extractor_type = typename traits::extractor_type;
  using allocator_type = typename traits::allocator_type;
  using is_associative = typename traits::is_associative;

  using allocator_traits = std::allocator_traits<allocator_type>;
  using key_type = typename container_type::key_type;
  using tag_type = Tag;

  using difference_type = typename container_type::difference_type;
  using value_type = typename container_type::value_type;
  using size_type = typename container_type::size_type;

  using const_iterator = typename container_type::const_iterator;
  using iterator = typename container_type::iterator;

  using const_reference = typename container_type::const_reference;
  using reference = typename container_type::reference;

  using const_pointer = typename container_type::const_pointer;
  using pointer = typename container_type::pointer;

  cache (cache const&) = delete;
  cache (cache&&) = delete;

  cache& operator = (cache const&) = delete;
  cache& operator = (cache&&) = delete;

  static cache& ref () noexcept {
    static cache instance;
    return instance;
  }

  const_iterator begin () const noexcept { return std::begin(this->container); }
  const_iterator end () const noexcept { return std::end(this->container); }

  size_type size () const noexcept { return this->container.size(); }
  bool empty () const noexcept { return this->container.empty(); }

  template <class ValueType>
  std::shared_ptr<core::add_const_t<T>> find (ValueType&& value) noexcept {
    std::lock_guard<std::mutex> lock { this->mutex };
    auto const& key = this->extractor(value);
    auto iter = this->container.find(key);
    if (iter != this->end()) { return iter->second.lock(); }
    return this->insert(::std::forward<ValueType>(value), is_associative { });
  }

private:
  iterator begin () noexcept { return std::begin(this->container); }
  iterator end () noexcept { return std::end(this->container); }

  template <class ValueType>
  std::shared_ptr<core::add_const_t<T>> insert (
    ValueType&& value,
    std::false_type&&
  ) noexcept {
    auto result = this->container.emplace(
      ::std::forward<ValueType>(value),
      std::shared_ptr<core::add_const_t<T>> { }
    );
    auto iter = std::get<0>(result);
    auto const& key = std::get<0>(*iter);
    std::shared_ptr<core::add_const_t<T>> shared {
      std::addressof(key),
      [this](T const* ptr) { this->remove(ptr, std::false_type { }); }
    };
    this->container[key] = shared;
    return shared;
  }

  template <class ValueType>
  std::shared_ptr<core::add_const_t<T>> insert (
    ValueType&& value,
    std::true_type&&
  ) noexcept {
    auto ptr = allocator_traits::allocate(this->allocator, 1);
    allocator_traits::construct(
      this->allocator,
      ptr,
      ::std::forward<ValueType>(value)
    );

    auto const& key = this->extractor(*ptr);
    std::shared_ptr<T> shared {
      ptr,
      [this](T const* ptr) { this->remove(ptr, std::true_type { }); }
    };
    auto result = this->container.emplace(key, shared);
    auto pair = std::get<0>(result);
    return pair->second.lock();
  }

  void remove (T const* ptr, std::false_type&&) noexcept {
    if (not ptr) { return; }
    std::lock_guard<std::mutex> lock { this->mutex };
    this->container.erase(*ptr);
  }

  void remove (T const* ptr, std::true_type&&) noexcept {
    if (not ptr) { return; }
    /* We don't want to keep the mutex locked after we've removed the weak_ptr */
    {
      std::lock_guard<std::mutex> lock { this->mutex };
      auto const& key = this->extractor(*ptr);
      this->container.erase(key);
    }
    allocator_traits::destroy(this->allocator, ptr);
    /* It is OK for us to const_cast here.
     * Because you CANNOT have an allocator to a T const, this means that it
     * can ONLY allocate a non-const pointer. And when is it ok to const_cast?
     * If the original object was declared as non-const (which it was). Now,
     * granted we've been copying the const pointer from shared_ptr to
     * shared_ptr, but, the object located at ptr has already been destroyed,
     * and now we're attempting to inform the allocator that the *mutable*
     * ptr it allocated is now to be deallocated. HENCE, THIS IS LEGAL.
     *
     * *DOINK* *DOINK*
     */
    allocator_traits::deallocate(this->allocator, const_cast<T*>(ptr), 1);
  }

  cache () = default;
  ~cache () = default;

  container_type container;
  allocator_type allocator;
  extractor_type extractor;
  std::mutex mutex;
};

}} /* namespace flyweight::v1 */

#endif /* FLYWEIGHT_CACHE_HPP */
