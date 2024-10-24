#pragma once

#include <atomic>
#include <cstdint>

namespace mvi {

struct alignas(alignof(std::atomic<uint64_t>)) BipBufferSPMCHeader {
  using ReadCursor = std::atomic<uint64_t>;

  std::atomic<uint64_t> write; // Write position
  std::atomic<uint64_t> last; // Marks the last valid byte in the buffer
  uint64_t readerCount; // Number of reader cursors (atomic<uint64_t>)
  uint64_t bufferSize; // Size of the buffer

  /// Returns a const reference to the reader cursor at the specified index
  const ReadCursor& reader(size_t index) const {
    return reinterpret_cast<const ReadCursor*>(this + sizeof(BipBufferSPMCHeader))[index];
  }

  /// Returns a reference to the reader cursor at the specified index
  ReadCursor& reader(size_t index) {
    return reinterpret_cast<ReadCursor*>(this + sizeof(BipBufferSPMCHeader))[index];
  }

  /// Returns a const pointer to the beginning of the circular buffer
  const uint8_t* buffer() const;

  /// Returns a pointer to the beginning of the circular buffer
  uint8_t* buffer();

  /**
   * Instantiate a BipBufferSPMCHeader from an existing block of memory.
   *
   * @param data Pointer to allocated memory where the header will be constructed.
   * @param size Size of the allocated memory block. The memory must be large
   *   enough to hold the full header structure (32 bytes), eight bytes per
   *   reader (`readerCount * 8`), plus at least one byte for the buffer.
   * @return Pointer to the initialized BipBufferSPMCHeader instance or
   *   nullptr if the parameters are invalid.
   */
  static BipBufferSPMCHeader* Create(uint8_t* data, size_t size, size_t readerCount);

private:
  BipBufferSPMCHeader() = default;
};

} // namespace mvi
