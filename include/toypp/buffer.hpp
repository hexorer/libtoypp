#ifndef TOYPP_BUFFER_HPP_
#define TOYPP_BUFFER_HPP_

#include <memory>
#include <type_traits>

namespace tpp {

template <typename T>
class Buffer {
 public:
  using value_type = std::remove_cv_t<std::remove_reference_t<T>>;

 private:
  const std::size_t _size;
  std::unique_ptr<value_type[]> _ptr;

 public:
  constexpr explicit Buffer(std::size_t size) : _size(size), _ptr(std::make_unique<value_type[]>(_size)) {}

  [[nodiscard]] constexpr value_type& at(std::size_t index) & { return _ptr[index]; }
  [[nodiscard]] constexpr const value_type& at(std::size_t index) const& { return _ptr[index]; }
  [[nodiscard]] constexpr value_type&& at(std::size_t index) && { return _ptr[index]; }
  [[nodiscard]] constexpr const value_type&& at(std::size_t index) const&& { return _ptr[index]; }

  [[nodiscard]] constexpr std::size_t size() const { return _size; }

  [[nodiscard]] constexpr value_type* begin() { return _ptr.get(); }
  [[nodiscard]] constexpr value_type* end() { return _ptr.get() + _size; }

  [[nodiscard]] constexpr const value_type* begin() const { return _ptr.get(); }
  [[nodiscard]] constexpr const value_type* end() const { return _ptr.get() + _size; }

  [[nodiscard]] constexpr value_type& operator[](std::size_t index) & { return _ptr[index]; }
  [[nodiscard]] constexpr const value_type& operator[](std::size_t index) const& { return _ptr[index]; }
  [[nodiscard]] constexpr value_type&& operator[](std::size_t index) && { return _ptr[index]; }
  [[nodiscard]] constexpr const value_type&& operator[](std::size_t index) const&& { return _ptr[index]; }
};

}  // namespace tpp

#endif  // TOYPP_BUFFER_HPP_
