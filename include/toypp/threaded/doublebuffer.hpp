#ifndef TOYPP_THREADED_DOUBLEBUFFER_HPP_
#define TOYPP_THREADED_DOUBLEBUFFER_HPP_

#include <atomic>
#include <mutex>

namespace tpp {

/**
 * @brief A generic DoubleBuffer implmentation to segregate
 *        read and write on available and back buffers.
 * 
 * DoubleBuffer is often used as a single item queue, where
 * one thread bakes its result into offline buffer, and when
 * ready, swaps it to become the available buffer,
 * so another can read it in parallel, and meanwhile
 * next result will be in write.
 * 
 * @tparam BufferT buffer type.
 */
template <typename BufferT>
class DoubleBuffer {
 public:
  using buffer_type = BufferT;

 private:
  buffer_type _buffers[2] = {};
  std::size_t _index = 0;
  std::atomic<bool> _waiting{false};

 public:
  DoubleBuffer() {}

  DoubleBuffer(buffer_type a, buffer_type b)
    : _buffers{std::move(a), std::move(b)}
  {}

  bool reader_arrive() noexcept {
    if (_waiting.load(std::memory_order_acquire)) {
      _index ^= 1ul;
      _waiting.store(false, std::memory_order_release);
      return true;
    }
    return false;
  }

  void writer_arrive_and_wait() noexcept {
    _waiting.store(true, std::memory_order_release);
    while (_waiting.load(std::memory_order_acquire)) {
      // do nothing.
    }
  }

  buffer_type& reader_buffer() noexcept {
    return _buffers[_index];
  }

  const buffer_type& reader_buffer() const noexcept {
    return _buffers[_index];
  }

  buffer_type& writer_buffer() noexcept {
    return _buffers[_index ^ 1ul];
  }

  const buffer_type& writer_buffer() const noexcept {
    return _buffers[_index ^ 1ul];
  }
};

}  // namespace tpp

#endif  // TOYPP_THREADED_DOUBLEBUFFER_HPP_
