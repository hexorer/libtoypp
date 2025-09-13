#ifndef TOYPP_FIXED_HASH_MAP_HPP_
#define TOYPP_FIXED_HASH_MAP_HPP_

#include <functional>
#include <memory>
#include <optional>

namespace tpp {

template <typename Key,
          typename Value,
          typename HashFn = std::hash<Key>,
          typename EqualFn = std::equal_to<Key>>
class ProbHashMap {
public:
  using key_type = Key;
  using value_type = Value;
  using hash_fn_type = HashFn;
  using equal_fn_type = EqualFn;

private:
  struct Entry {
    std::optional<Key> key;
    Value value;
  };

  std::unique_ptr<Entry[]> _entries;
  std::size_t _size;

public:
  explicit constexpr ProbHashMap() {}
};

} // namespace tpp

#endif  // TOYPP_FIXED_HASH_MAP_HPP_
