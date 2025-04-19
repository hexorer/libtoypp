#ifndef TOYPP_IMMUTABLE_STRING_HPP_
#define TOYPP_IMMUTABLE_STRING_HPP_

#include <atomic>
#include <cstring>
#include <cstdlib>
#include <string_view>

namespace tpp {

/** An immutable string with builtin shared reference count. (similar to `std::shared_ptr`)
 * NOTE: substr also used refrence counting.
 */
class ImmutableString {
  using char_type = char;

  static_assert(sizeof(char_type) == 1);

  static constexpr std::size_t k_short_max_size = 2 * sizeof(char_type*) - sizeof(std::uint32_t);
  static_assert(k_short_max_size > 0);

  struct Data {
    std::atomic<std::uint32_t> refcount;
  };

  union {
    struct Longer {
      std::uint32_t size;
      std::uint32_t offset;
      Data* ptr; // reinterpret_cast<char_type*>(ptr) + sizeof(Data) == data()
    } longer;

    struct Shorter {
      std::uint32_t size;
      char_type bytes[k_short_max_size];
    } shorter;
  } _value;

  explicit ImmutableString(std::size_t size) {
    _value.shorter.size = size;
    if (size > k_short_max_size) {
      _value.longer.offset = 0;
      _value.longer.ptr = reinterpret_cast<Data*>(std::aligned_alloc(alignof(Data), sizeof(Data) + size));
      _value.longer.ptr->refcount = 1;
    }
  }

  char_type* raw_long_str_ptr() {
    return reinterpret_cast<char_type*>(_value.longer.ptr) + sizeof(Data);
  }

  const char_type* raw_long_str_ptr() const {
    return reinterpret_cast<const char_type*>(_value.longer.ptr) + sizeof(Data);
  }

  void destroy() {
    if (_value.longer.size <= k_short_max_size) {
      return;
    }

    if (_value.longer.ptr->refcount.fetch_sub(1u) == 1u)
    {
      std::free(_value.longer.ptr);
    }

    _value.shorter.size = 0;
  }

 public:
  ImmutableString() {
    _value.shorter.size = 0;
  }
  ImmutableString(const char* c_str) : ImmutableString(std::string_view(c_str)) {}
  ImmutableString(std::string_view str) : ImmutableString(str.size()) {
    std::memcpy(const_cast<char_type*>(data()), str.data(), str.size());
  }

  ImmutableString(const ImmutableString& other) {
    if (other._value.shorter.size > k_short_max_size)
    {
      other._value.longer.ptr->refcount.fetch_add(1u);
    }
    std::memcpy(&_value, &other._value, sizeof(_value));
  }

  ImmutableString(ImmutableString&& other) {
    std::memcpy(&_value, &other._value, sizeof(_value));
    other._value.shorter.size = 0;
  }

  ImmutableString& operator=(const ImmutableString& other) {
    if (this == &other) {
      return *this;
    }
    if (other._value.shorter.size > k_short_max_size)
    {
      other._value.longer.ptr->refcount.fetch_add(1u);
    }
    destroy();
    std::memcpy(&_value, &other._value, sizeof(_value));
    return *this;
  }

  ImmutableString& operator=(ImmutableString&& other) {
    if (this == &other) {
      return *this;
    }
    if (other._value.shorter.size > k_short_max_size)
    {
      other._value.longer.ptr->refcount.fetch_add(1u);
    }
    destroy();
    std::memcpy(&_value, &other._value, sizeof(_value));
    other._value.shorter.size = 0;
    return *this;
  }

  ~ImmutableString() {
    destroy();
  }

  std::size_t size() const { return _value.shorter.size; }
  const char_type* data() const {
    return (size() <= k_short_max_size)
      ? _value.shorter.bytes
      : (raw_long_str_ptr() + _value.longer.offset);
  }

  char_type at(std::size_t index) const { return data()[index]; }

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
  ImmutableString substr(std::size_t offset, std::size_t size = (1ul << 32)) const {
    offset = std::min(offset, static_cast<std::size_t>(_value.shorter.size));
    size = std::min(size, _value.shorter.size - offset);

    if (size <= 0) {
      return ImmutableString();
    }

    if (size <= k_short_max_size) {
      ImmutableString ret;
      ret._value.shorter.size = size;
      std::memcpy(ret._value.shorter.bytes, data() + offset, size);
      return ret;
    }

    ImmutableString ret = *this;
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
    ImmutableString ret{size() + other.size()};
    char_type* dest = const_cast<char_type*>(ret.data());
    std::memcpy(dest, data(), size());
    std::memcpy(dest + size(), other.data(), other.size());
    return ret;
  }

  ImmutableString prepend(const char* other) const {
    return prepend(std::string_view(other));
  }

  ImmutableString prepend(const ImmutableString& other) const {
    return prepend(other.as_strview());
  }

  ImmutableString prepend(std::string_view other) const {
    ImmutableString ret{other.size() + size()};
    char_type* dest = const_cast<char_type*>(ret.data());
    std::memcpy(dest, other.data(), other.size());
    std::memcpy(dest + other.size(), data(), size());
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

}  // namespace tpp

#endif  // TOYPP_IMMUTABLE_STRING_HPP_
