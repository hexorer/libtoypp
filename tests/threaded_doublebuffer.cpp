#include <thread>
#include <vector>

#include <catch2/catch_all.hpp>

#include "toypp/threaded/doublebuffer.hpp"

TEST_CASE("tpp::DoubleBuffer") {
  tpp::DoubleBuffer<std::vector<int>> dbuff{};

  auto* buff_a_ptr = std::addressof(dbuff.reader_buffer());
  auto* buff_b_ptr = std::addressof(dbuff.writer_buffer());

  REQUIRE(buff_a_ptr != buff_b_ptr);

  for (int i = 0; i < 10; ++i) {
    std::thread a([&] { dbuff.writer_arrive_and_wait(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    REQUIRE(buff_a_ptr == std::addressof(dbuff.reader_buffer()));
    REQUIRE(buff_b_ptr == std::addressof(dbuff.writer_buffer()));

    dbuff.reader_arrive();

    a.join();

    REQUIRE(buff_b_ptr == std::addressof(dbuff.reader_buffer()));
    REQUIRE(buff_a_ptr == std::addressof(dbuff.writer_buffer()));

    std::swap(buff_a_ptr, buff_b_ptr); // for the next round...
  }
}

