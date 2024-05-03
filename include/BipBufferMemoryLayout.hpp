#pragma once

#include <atomic>
#include <cstddef>

namespace mvi {

struct alignas(alignof(std::atomic<size_t>)) BipBufferMemoryLayout {
  std::atomic<size_t> read; // Read position
  std::atomic<size_t> write; // Write position
  std::atomic<size_t> last; // Marks the last valid byte in the buffer
  size_t bufferSize; // Size of the buffer
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc99-extensions"
  uint8_t buffer[]; // Circular buffer
#pragma clang diagnostic pop

  /**
   * Instantiate a BipBufferMemoryLayout from an existing block of memory.
   *
   * @param data Pointer to allocated memory where the layout will be constructed.
   * @param size Size of the allocated memory block. The memory must be large
   *   enough to hold the full layout structure (32 bytes), plus at least one byte
   *   for the buffer.
   * @return Pointer to the initialized BipBufferMemoryLayout instance or nullptr
   *   if the parameters are invalid.
   */
  static BipBufferMemoryLayout* Create(uint8_t* data, size_t size);
};

} // namespace mvi
