#include <iostream>
#include <thread>

#include <benchmark/benchmark.h>

#include "toypp/threaded/doublebuffer.hpp"

static void benchmark_reader_writer_latency(benchmark::State& state)
{
  tpp::DoubleBuffer<std::uint64_t> doublebuffer{};
  std::atomic<std::uint64_t> running{false};

  std::thread thread{[&] {
    running.store(true, std::memory_order_relaxed);
    while (running.load(std::memory_order_relaxed))
    {
      while (!doublebuffer.reader_arrive()) {}
    }
  }};

  while (!running.load(std::memory_order_acquire))
  {
    benchmark::DoNotOptimize(running);
  }

  std::size_t count = 0;
  for (auto _ : state)
  {
    doublebuffer.writer_arrive_and_wait();
    ++count;
  }

  running.store(false, std::memory_order_relaxed);
  doublebuffer.writer_arrive_and_wait();
  thread.join();

  state.SetItemsProcessed(count);
}
BENCHMARK(benchmark_reader_writer_latency);

BENCHMARK_MAIN();
