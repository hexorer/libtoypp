#include <catch2/catch_all.hpp>

#include "toypp/buffer.hpp"

TEST_CASE("tpp::Buffer") {
  constexpr std::size_t count = 10;
  auto buffer = tpp::Buffer<std::string>(count);

  CHECK(buffer.size() == count);
  CHECK(buffer.begin() != buffer.end());
  CHECK(buffer.begin() + count == buffer.end());

  for (std::size_t index = 0; index < buffer.size(); ++index) {
    buffer[index] = std::to_string(index);
  }

  for (std::size_t index = 0; index < buffer.size(); ++index) {
    CHECK(buffer[index] == std::to_string(index));
  }

  {
    std::size_t index = 0;
    for (std::string& item : buffer) {
      CHECK(item == std::to_string(index++));
    }
  }
}
