#ifndef TOYPP_IMMUTABLE_STRING_HPP_
#define TOYPP_IMMUTABLE_STRING_HPP_

#include <atomic>
#include <cstring>
#include <cstdlib>
#include <string_view>

namespace tpp {

/** An immutable string with builtin shared reference count. (similar to `std::shared_ptr`)
 * NOTE: substr also used refrence counting.
 * NOTE: it can also be used as a string view without refcounting. (useful for literal strs)
 */
class ImmutableString {
  using char_type = char;

  static_assert(sizeof(char_type) == 1);

  static constexpr std::size_t k_short_max_size = 2 * sizeof(char_type*) - sizeof(std::uint32_t) - 1;
  static_assert(k_short_max_size > 0);

  struct Data {
    std::atomic<std::uint32_t> refcount;
  };

  union {
    struct Longer {
      std::uint32_t size;
      std::uint32_t offset;
      void* ptr; // static_cast<char_type*>(ptr) + sizeof(Data) == data()
    } longer;

    struct Shorter {
      std::uint32_t size;
      char_type bytes[k_short_max_size];
    } shorter;
  } _value;

  explicit ImmutableString(std::size_t size) {
    _value.shorter.size = size;
    _value.shorter.bytes[0] = '\0';
    if (size > k_short_max_size) {
      _value.longer.offset = 0;
      const std::size_t allocation_size = sizeof(Data) + size + 1;
      constexpr std::size_t alignment = alignof(Data);
      _value.longer.ptr = std::aligned_alloc(alignment,  (allocation_size / alignment + 1) * alignment);
      static_cast<Data*>(_value.longer.ptr)->refcount.store(1u, std::memory_order_relaxed);
    }
  }

 public:
  // /**
  //  * makes an ImmutableString without refcount'ing and allocation
  //  * which reuses the same pointer as the given string view.
  //  * NOTE: if the given str fits in the small buffer, it will be copied.
  //  */
  // static ImmutableString make_view(std::string_view str) {
  //   if (str.size() <= k_short_max_size) {
  //     return ImmutableString(str);
  //   }
  //   ImmutableString ret;
  //   ret._value.longer.size = str.size();
  //   ret._value.longer.offset = str.size();
  //   ret._value.longer.ptr = const_cast<char*>(str.data());
  //   return ret;
  // }

  ImmutableString() noexcept {
    _value.shorter.size = 0;
    _value.shorter.bytes[0] = '\0';
  }
  ImmutableString(const char* c_str) : ImmutableString(std::string_view(c_str)) {}
  ImmutableString(std::string_view str) : ImmutableString(str.size()) {
    std::memcpy(const_cast<char_type*>(data()), str.data(), str.size());
  }

  ImmutableString(const ImmutableString& other) {
    if (other._value.shorter.size > k_short_max_size) {
      static_cast<Data*>(other._value.longer.ptr)->refcount.fetch_add(1u, std::memory_order_relaxed);
    }
    _value = other._value;
  }

  ImmutableString(ImmutableString&& other) noexcept {
    _value = other._value;
    other._value.shorter.size = 0;
  }

  ImmutableString& operator=(const ImmutableString& other) {
    if (other._value.shorter.size > k_short_max_size) {
      static_cast<Data*>(other._value.longer.ptr)->refcount.fetch_add(1u, std::memory_order_relaxed);
    }
    if (_value.shorter.size > k_short_max_size) {
      if (static_cast<Data*>(_value.longer.ptr)->refcount.fetch_sub(1u, std::memory_order_relaxed) == 1u) {
        std::free(_value.longer.ptr);
      }
    }
    _value = other._value;
    return *this;
  }

  ImmutableString& operator=(ImmutableString&& other) noexcept {
    std::swap(_value, other._value);
    if (data() == other.data()) {
      other._value.shorter.size = 0;
    }
    return *this;
  }

  ~ImmutableString() {
    if (_value.shorter.size <= k_short_max_size) {
      return;
    }
    if (static_cast<Data*>(_value.longer.ptr)->refcount.fetch_sub(1u, std::memory_order_relaxed) == 1u) {
      std::free(_value.longer.ptr);
    }
  }

  std::size_t size() const { return _value.shorter.size; }

  const char_type* data() const {
    if (_value.shorter.size <= k_short_max_size) {
      return _value.shorter.bytes;
    }

    // if (_value.longer.size == _value.longer.offset) {
    //   return static_cast<const char_type*>(_value.longer.ptr);
    // }

    return static_cast<const char_type*>(_value.longer.ptr) + sizeof(Data) + _value.longer.offset;
  }

  char_type at(std::size_t index) const { return data()[index]; }

  const char_type* begin() const { return data(); }
  const char_type* end() const { return data() + size(); }

  /** makes a shared copy pointing to the region specified.
   *
   * the following calculations are applied to make sure
   * in case of out of bounds offset and size,
   * the values are clamped down to the appropriate value,
   * thus instead of failing, the operation always succeeds.
   *
   * offset = min(offset, size())
   * size = min(size, size() - min(offset, size()))
   *
   * NOTE: since the object is immutable, there is no point in using refcount
   *       when the result fits in the desired short string optimization size.
   */
  ImmutableString substr(std::uint32_t offset, std::uint32_t size = ((1ul << 32) - 1ul)) const {
    ImmutableString ret;

    if (offset >= _value.shorter.size) {
      return ret;
    }

    if (size > _value.shorter.size - offset) {
      size = _value.shorter.size - offset;
    }

    if (size <= k_short_max_size) {
      ret._value.shorter.size = size;
      std::memcpy(ret._value.shorter.bytes, data() + offset, size);
      return ret;
    }

    // if (_value.longer.size == _value.longer.offset) {
    //   ret._value.longer.size = size;
    //   ret._value.longer.offset = size;
    //   ret._value.longer.ptr = static_cast<char_type*>(_value.longer.ptr) + offset;
    //   return ret;
    // }

    ret._value.longer.ptr = _value.longer.ptr;
    ret._value.longer.size = size;
    ret._value.longer.offset = offset;

    return ret;
  }

  ImmutableString append(const char* other) const {
    return append(std::string_view(other));
  }

  ImmutableString append(const ImmutableString& other) const {
    return append(other.as_strview());
  }

  ImmutableString append(std::string_view other) const {
    if (other.size() <= 0) {
      return *this;
    }
    ImmutableString ret{size() + other.size()};
    char_type* dest = const_cast<char_type*>(ret.data());
    std::memcpy(dest, data(), size());
    std::memcpy(dest + size(), other.data(), other.size());
    dest[ret.size()] = '\0';
    return ret;
  }

  ImmutableString prepend(const char* other) const {
    return prepend(std::string_view(other));
  }

  ImmutableString prepend(const ImmutableString& other) const {
    return prepend(other.as_strview());
  }

  ImmutableString prepend(std::string_view other) const {
    if (other.size() <= 0) {
      return *this;
    }
    ImmutableString ret{other.size() + size()};
    char_type* dest = const_cast<char_type*>(ret.data());
    std::memcpy(dest, other.data(), other.size());
    std::memcpy(dest + other.size(), data(), size());
    dest[ret.size()] = '\0';
    return ret;
  }

  bool equals(const char* c_string) const {
    return size() == std::strlen(c_string) && std::memcmp(data(), c_string, size()) == 0;
  }

  bool equals(std::string_view strview) const {
    return size() == strview.size() && std::memcmp(data(), strview.data(), size()) == 0;
  }

  bool equals(const ImmutableString& other) const {
    return size() == other.size() && std::memcmp(data(), other.data(), size()) == 0;
  }

  std::string_view as_strview() const {
    return std::string_view(data(), size());
  }

  char_type operator[](std::size_t index) const { return at(index); }

  friend ImmutableString operator+(const ImmutableString& lhs, const char* rhs) { return lhs.append(rhs); }
  friend ImmutableString operator+(const ImmutableString& lhs, std::string_view rhs) { return lhs.append(rhs); }
  friend ImmutableString operator+(const ImmutableString& lhs, const ImmutableString& rhs) { return lhs.append(rhs); }

  friend ImmutableString operator+(const char* lhs, const ImmutableString& rhs) { return rhs.prepend(lhs); }
  friend ImmutableString operator+(std::string_view lhs, const ImmutableString& rhs) { return rhs.prepend(lhs); }

  bool operator==(const char* other) const { return equals(other); }
  bool operator!=(const char* other) const { return !equals(other); }

  bool operator==(std::string_view other) const { return equals(other); }
  bool operator!=(std::string_view other) const { return !equals(other); }

  bool operator==(const ImmutableString& other) const { return equals(other); }
  bool operator!=(const ImmutableString& other) const { return !equals(other); }

  friend std::ostream& operator<<(std::ostream& ostream, const ImmutableString& str) {
    ostream << str.as_strview();
    return ostream;
  }

  operator std::string_view() const { return as_strview(); }
};

static_assert(sizeof(ImmutableString) == sizeof(std::uint64_t) * 2);

}  // namespace tpp

#endif  // TOYPP_IMMUTABLE_STRING_HPP_
