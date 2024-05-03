#pragma once

#include "BipBufferMemoryLayout.hpp"
#include "BipBufferWriterReservation.hpp"

#include <cstddef>
#include <memory>

namespace mvi {

/**
 * A BipBufferWriter is used to write data into a BipBufferMemoryLayout. It
 * provides a method to reserve a contiguous block of memory in the buffer,
 * represented as a BipBufferWriterReservation. The reservation is committed
 * when the unique_ptr goes out of scope.
 */
class BipBufferWriter {
public:
  /// Construct a BipBufferWriter as the exclusive writer for a BipBufferMemoryLayout.
  explicit BipBufferWriter(BipBufferMemoryLayout& layout) : layout_(layout) {}

  /**
   * Tries to reserve a contiguous block of memory in the buffer. If successful,
   * returns a unique_ptr to a reservation. If not enough space is available,
   * nullptr is returned.
   *
   * Only one reservation can be active at a time. Attempting to reserve space
   * while a reservation is active will result in undefined behavior. The
   * reservation is committed when the unique_ptr goes out of scope.
   *
   * @param length The number of bytes to reserve. If there is not enough
   *   contiguous space available, nullptr is returned.
   * @return A BipBufferWriterReservation if space was reserved, nullptr
   *   otherwise.
   */
  std::unique_ptr<BipBufferWriterReservation> reserve(size_t length);

private:
  BipBufferMemoryLayout& layout_;

  friend class BipBufferWriterReservation;

  // Returns a pointer to the start of the circular buffer (after the header).
  uint8_t* data();

  // Commits previously reserved space: updates the `last` and `write` positions
  // in the layout header.
  void commit(size_t start, size_t len, bool wraparound);
};

} // namespace mvi
