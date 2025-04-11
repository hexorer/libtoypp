#ifndef TOYPP_THREADED_PUBSUB_QUEUE_HPP_
#define TOYPP_THREADED_PUBSUB_QUEUE_HPP_

#include <atomic>
#include <memory>
#include <optional>
#include <shared_mutex>
#include <type_traits>

namespace tpp {

template <typename T>
class PubSubQueue {
  using value_type = std::remove_cv_t<std::remove_reference_t<T>>;

  static_assert(
    std::is_copy_assignable_v<value_type>,
    "type must be copyable so multiple subscribers read the same value."
  );

  struct Node {
    std::optional<value_type> value;
    std::shared_ptr<Node> next{};
  };

  std::shared_mutex _mutex;
  std::shared_ptr<Node> _tail;

 public:
  class Subscription {
    friend class PubSubQueue;

    std::shared_ptr<Node> _head;

    explicit Subscription(std::shared_ptr<Node> node) noexcept : _head(std::move(node)) {}

   public:
    std::optional<value_type> dequeue() {
      auto head_next = std::atomic_load(&_head->next);
      if (!head_next) {
        return std::nullopt;
      }
      _head = std::move(head_next);
      return _head->value;
    }
  };

  PubSubQueue() : _tail(std::make_shared<Node>()) {}

  PubSubQueue(const PubSubQueue&) = delete;
  PubSubQueue(PubSubQueue&&) noexcept = delete;

  PubSubQueue& operator=(const PubSubQueue&) = delete;
  PubSubQueue& operator=(PubSubQueue&&) noexcept = delete;

  ~PubSubQueue() = default;

  void publish(value_type value) {
    std::lock_guard lock{_mutex};
    auto prev = std::exchange(_tail, std::make_shared<Node>(Node{.value = std::move(value)}));
    std::atomic_store(&prev->next, _tail);
  }

  Subscription subscribe() {
    std::shared_lock lock{_mutex};
    return Subscription(_tail);
  }
};

}  // namespace tpp

#endif  // TOYPP_THREADED_PUBSUB_QUEUE_HPP_
