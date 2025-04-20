#ifndef TOYPP_SHAREDPTR_HPP_
#define TOYPP_SHAREDPTR_HPP_

#include <atomic>
#include <functional>
#include <type_traits>
#include <utility>

namespace tpp {

namespace details {
template <typename T>
struct is_unbounded_array : std::false_type {};

template <typename T>
struct is_unbounded_array<T[]> : std::true_type {};

struct SharedPtrShared {
  class AnyPtr {
    void* _ptr = nullptr;
  public:
    template <typename T>
    AnyPtr(T* ptr) : _ptr(ptr) {}

    template <typename Y>
    operator Y*() const { return static_cast<Y*>(_ptr); }
  };

  std::atomic<std::size_t> ptr_refcount = 1;
  std::atomic<std::size_t> shared_refcount = 1;
  std::function<void(AnyPtr)> deleter;
};
}  // namespace details

template <typename T>
class WeakPtr;

template <typename T>
class SharedPtr {
  using value_type = T;

  static_assert(!std::is_reference_v<T>, "SharedPtr does not support reference types.");

  static constexpr auto k_default_deleter = [](value_type* ptr) {
    if constexpr (std::is_array_v<value_type>) {
      delete[] ptr;
    } else {
      delete ptr;
    }
  };

  value_type* _ptr;
  details::SharedPtrShared* _shared;

  template <typename U>
  friend class SharedPtr;

  template <typename U>
  friend class WeakPtr;

  void destroy() {
    if (!_shared) {
      return;
    }
    if (_shared->ptr_refcount.fetch_sub(1) == 1) {
      _shared->deleter(_ptr);
      _ptr = nullptr;
    }
    if (_shared->shared_refcount.fetch_sub(1) == 1) {
      delete _shared;
      _shared = nullptr;
    }
  }

  explicit SharedPtr(value_type* ptr, details::SharedPtrShared* shared) noexcept : _ptr(ptr), _shared(shared) {}

 public:
  SharedPtr() noexcept : _ptr(nullptr), _shared(nullptr) {}
  SharedPtr(std::nullptr_t  /* ptr */) noexcept : SharedPtr() {}

  template <typename F>
  SharedPtr(std::nullptr_t /* ptr */, F&& /* deleter */) noexcept : SharedPtr() {}

  SharedPtr(value_type* ptr) : SharedPtr(ptr, k_default_deleter) {}

  template <typename Y>
  SharedPtr(Y* ptr) : SharedPtr(ptr, k_default_deleter) {}

  template <typename F, std::enable_if_t<std::is_invocable_v<F, value_type*>, int> = 0>
  SharedPtr(value_type* ptr, F&& deleter) : _ptr(ptr), _shared(ptr ? new details::SharedPtrShared{.deleter = std::forward<F>(deleter)} : nullptr) {}

  template <typename Y, typename F, std::enable_if_t<std::is_invocable_v<F, value_type*>, int> = 0>
  SharedPtr(Y* ptr, F&& deleter) : _ptr(ptr), _shared(ptr ? new details::SharedPtrShared{.deleter = std::forward<F>(deleter)} : nullptr) {}

  SharedPtr(const SharedPtr& other) noexcept : _ptr(other._ptr), _shared(other._shared) {
    if (_shared) {
      other._shared->ptr_refcount.fetch_add(1);
      other._shared->shared_refcount.fetch_add(1);
    }
  }
  SharedPtr(SharedPtr&& other) noexcept
    : _ptr(std::exchange(other._ptr, nullptr))
    , _shared(std::exchange(other._shared, nullptr))
  {}

  template <typename Y, std::enable_if_t<std::is_convertible_v<typename SharedPtr<Y>::value_type*, value_type*>, int> = 0>
  SharedPtr(const SharedPtr<Y>& other) noexcept : _ptr(static_cast<value_type*>(other._ptr)), _shared(other._shared) {
    if (_shared) {
      other._shared->ptr_refcount.fetch_add(1);
      other._shared->shared_refcount.fetch_add(1);
    }
  }

  template <typename Y, std::enable_if_t<std::is_convertible_v<typename SharedPtr<Y>::value_type*, value_type*>, int> = 0>
  SharedPtr(SharedPtr<Y>&& other) noexcept
    : _ptr(static_cast<value_type*>(std::exchange(other._ptr, nullptr)))
    , _shared(std::exchange(other._shared, nullptr))
  {}

  SharedPtr& operator=(const SharedPtr& other) noexcept {
    SharedPtr(other).swap(*this);
    return *this;
  }
  SharedPtr& operator=(SharedPtr&& other) noexcept {
    SharedPtr(std::move(other)).swap(*this);
    return *this;
  }

  template <typename Y>
  SharedPtr& operator=(const SharedPtr<Y>& other) noexcept {
    SharedPtr(other).swap(*this);
    return *this;
  }
  template <typename Y>
  SharedPtr& operator=(SharedPtr<Y>&& other) noexcept {
    SharedPtr(std::move(other)).swap(*this);
    return *this;
  }

  ~SharedPtr() { destroy(); }

  value_type* get() const noexcept { return _ptr; }
  std::size_t use_count() const noexcept { return _shared ? _shared->ptr_refcount.load() : 0; }

  void reset() { reset(nullptr); }
  void reset(value_type* ptr) { reset(ptr, k_default_deleter); }

  template <typename F>
  void reset(value_type* ptr, F&& deleter) { SharedPtr(ptr, std::forward<F>(deleter)).swap(*this); }

  void swap(SharedPtr& other) noexcept {
    std::swap(_ptr, other._ptr);
    std::swap(_shared, other._shared);
  }

  bool operator==(const SharedPtr& other) const noexcept { return _ptr == other._ptr; }
  bool operator!=(const SharedPtr& other) const noexcept { return _ptr != other._ptr; }

  value_type* operator->() const noexcept { return _ptr; }
  value_type& operator*() const noexcept { return *_ptr; }
  value_type& operator[](std::size_t index) const noexcept { return _ptr[index]; }

  operator bool() const noexcept { return static_cast<bool>(_ptr); }
};

template <typename T>
class WeakPtr {
  using value_type = std::remove_cv_t<std::remove_reference_t<T>>;
  using shared_type = details::SharedPtrShared;

  value_type* _ptr;
  shared_type* _shared;

  void destroy() {
    if (!_shared) {
      return;
    }
    if (_shared->shared_refcount.fetch_sub(1) == 1) {
      delete _shared;
      _shared = nullptr;
    }
  }

 public:
  WeakPtr() noexcept : _ptr(nullptr), _shared(nullptr) {}

  WeakPtr(const WeakPtr& other) noexcept : _ptr(other._ptr), _shared(other._shared) {
    if (_shared) {
      _shared->shared_refcount.fetch_add(1);
    }
  }
  WeakPtr(WeakPtr&& other) noexcept
    : _ptr(std::exchange(other._ptr, nullptr))
    , _shared(std::exchange(other._shared, nullptr))
  {}

  template <typename Y>
  WeakPtr(const WeakPtr<Y>& other) noexcept : _ptr(static_cast<value_type*>(other._ptr)), _shared(other._shared) {
    if (_shared) {
      _shared->shared_refcount.fetch_add(1);
    }
  }
  template <typename Y>
  WeakPtr(WeakPtr<Y>&& other) noexcept
    : _ptr(static_cast<value_type*>(std::exchange(other._ptr, nullptr)))
    , _shared(std::exchange(other._shared, nullptr))
  {}

  WeakPtr& operator=(const WeakPtr& other) noexcept {
    WeakPtr(other).swap(*this);
    return *this;
  }
  WeakPtr& operator=(WeakPtr&& other) noexcept {
    WeakPtr(std::move(other)).swap(*this);
    return *this;
  }

  template <typename Y>
  WeakPtr& operator=(const WeakPtr<Y>& other) noexcept {
    WeakPtr(other).swap(*this);
    return *this;
  }
  template <typename Y>
  WeakPtr& operator=(WeakPtr<Y>&& other) noexcept {
    WeakPtr(std::move(other)).swap(*this);
    return *this;
  }

  ~WeakPtr() { destroy(); }

  WeakPtr(const SharedPtr<T>& other) noexcept : _ptr(static_cast<value_type*>(other._ptr)), _shared(other._shared) {
    if (_shared) {
      _shared->shared_refcount.fetch_add(1);
    }
  }

  WeakPtr& operator=(const SharedPtr<T>& other) noexcept {
    WeakPtr(other).swap(*this);
    return *this;
  }

  template <typename Y>
  WeakPtr(const SharedPtr<Y>& other) noexcept : _ptr(static_cast<value_type*>(other._ptr)), _shared(other._shared) {
    if (_shared) {
      _shared->shared_refcount.fetch_add(1);
    }
  }

  template <typename Y>
  WeakPtr& operator=(const SharedPtr<Y>& other) noexcept {
    WeakPtr(other).swap(*this);
    return *this;
  }

  std::size_t use_count() const noexcept { return _shared ? _shared->ptr_refcount.load() : 0; }

  bool expired() const noexcept { return use_count() <= 0; }

  void swap(WeakPtr& other) noexcept {
    std::swap(_ptr, other._ptr);
    std::swap(_shared, other._shared);
  }

  SharedPtr<value_type> lock() const noexcept {
    if (!_shared) {
      return SharedPtr<value_type>{};
    }

    std::size_t refcount = _shared->ptr_refcount.load();
    do {
      if (refcount <= 0) {
        return SharedPtr<value_type>{};
      }
    } while (!_shared->ptr_refcount.compare_exchange_strong(refcount, refcount + 1));

    _shared->shared_refcount.fetch_add(1);

    return SharedPtr<value_type>(_ptr, _shared);
  }
};

template <typename T, typename ...Args>
inline std::enable_if_t<!details::is_unbounded_array<T>::value, SharedPtr<T>> make_shared(Args&& ...args) {
  using value_type = std::remove_cv_t<std::remove_reference_t<T>>;
  return SharedPtr<T>(new value_type(std::forward<Args>(args)...));
}

template <typename T>
inline std::enable_if_t<details::is_unbounded_array<T>::value, SharedPtr<T>> make_shared(std::size_t size) {
  using value_type = std::remove_cv_t<std::remove_reference_t<T>>;
  return SharedPtr<T>(new value_type[size]);
}

}  // namespace tpp

#endif  // TOYPP_SHAREDPTR_HPP_
