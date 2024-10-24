#pragma once

#include <atomic>
#include <cstdint>

namespace mvi {

/**
 * A header structure for a BipBuffer that is used to manage the buffer's
 * read, write, and end of data positions. The buffer is prefixed with this
 * structure in memory.
 */
struct alignas(alignof(std::atomic<uint64_t>)) BipBufferHeader {
  std::atomic<uint64_t> read; // Read position
  std::atomic<uint64_t> write; // Write position
  std::atomic<uint64_t> last; // Marks the last valid byte in the buffer
  uint64_t bufferSize; // Size of the buffer

  /// Returns a const pointer to the beginning of the circular buffer
  const uint8_t* buffer() const;

  /// Returns a pointer to the beginning of the circular buffer
  uint8_t* buffer();

  /**
   * Instantiate a BipBufferHeader from an existing block of memory.
   *
   * @param data Pointer to allocated memory where the header will be constructed.
   * @param size Size of the allocated memory block. The memory must be large
   *   enough to hold the full header structure (32 bytes), plus at least one
   *   byte for the buffer.
   * @return Pointer to the initialized BipBufferHeader instance or nullptr if
   *   the parameters are invalid.
   */
  static BipBufferHeader* Create(uint8_t* data, size_t size);

private:
  BipBufferHeader() = default;
};

} // namespace mvi
