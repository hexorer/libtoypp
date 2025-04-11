#include <cstdint>
#include <thread>
#include <vector>

#include <catch2/catch_all.hpp>

#include "toypp/threaded/pubsub_queue.hpp"

TEST_CASE("tpp::PubSubQueue simple") {
  tpp::PubSubQueue<std::size_t> pubsub_queue{};

  auto subscriber_a = pubsub_queue.subscribe();
  auto subscriber_b = pubsub_queue.subscribe();

  CHECK(subscriber_a.dequeue() == std::nullopt);
  CHECK(subscriber_b.dequeue() == std::nullopt);

  constexpr std::size_t count = 10;

  for (std::size_t number = 0; number < count; ++number) {
    pubsub_queue.publish(number);
  }

  for (std::size_t number = 0; number < count; ++number) {
    CHECK(subscriber_a.dequeue() == number);
  }
  CHECK(subscriber_a.dequeue() == std::nullopt);

  for (std::size_t number = 0; number < count; ++number) {
    CHECK(subscriber_b.dequeue() == number);
  }
  CHECK(subscriber_b.dequeue() == std::nullopt);

  pubsub_queue.publish(count);

  CHECK(subscriber_a.dequeue() == count);
  CHECK(subscriber_b.dequeue() == count);

  CHECK(subscriber_a.dequeue() == std::nullopt);
  CHECK(subscriber_b.dequeue() == std::nullopt);
}

TEST_CASE("tpp::PubSubQueue late-subscribe") {
  tpp::PubSubQueue<std::size_t> pubsub_queue{};

  constexpr std::size_t count = 10;

  auto subscriber_a = pubsub_queue.subscribe();
  CHECK(subscriber_a.dequeue() == std::nullopt);

  for (std::size_t number = 0; number < (count / 2); ++number) {
    pubsub_queue.publish(number);
  }

  auto subscriber_b = pubsub_queue.subscribe();
  CHECK(subscriber_b.dequeue() == std::nullopt);

  for (std::size_t number = (count / 2); number < count; ++number) {
    pubsub_queue.publish(number);
  }

  for (std::size_t number = 0; number < (count / 2); ++number) {
    CHECK(subscriber_a.dequeue() == number);
  }

  for (std::size_t number = (count / 2); number < count; ++number) {
    CHECK(subscriber_a.dequeue() == number);
    CHECK(subscriber_b.dequeue() == number);
  }

  CHECK(subscriber_a.dequeue() == std::nullopt);
  CHECK(subscriber_b.dequeue() == std::nullopt);
}

TEST_CASE("tpp::PubSubQueue stress") {
  tpp::PubSubQueue<std::size_t> pubsub_queue{};

  constexpr std::size_t count = 1000;
  constexpr std::size_t thread_count = 10;

  std::vector<std::thread> threads;
  threads.resize(thread_count);

  std::atomic<bool> failed{false};
  for (std::size_t index = 0; index < thread_count; ++index) {
    threads[index] = std::thread([&failed, subscriber=pubsub_queue.subscribe()]() mutable {
      std::size_t last = 0;
      while (last != count) {
        std::optional<std::size_t> res = subscriber.dequeue();
        if (!res) {
          continue;
        }
        if (++last != res) {
          failed = true;
          break;
        }
      }
      if (subscriber.dequeue() != std::nullopt) {
        failed = true;
      }
    });
  }

  for (std::size_t number = 1; number <= count; ++number) {
    pubsub_queue.publish(number);
  }

  for (std::thread& thread : threads) {
    thread.join();
  }

  CHECK(!failed);
}
