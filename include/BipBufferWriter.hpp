#pragma once

#include "BipBufferHeader.hpp"
#include "BipBufferWriterReservation.hpp"

#include <cstddef>
#include <memory>

namespace mvi {

/**
 * A BipBufferWriter is used to write data into a bipartite circular buffer
 * prefixed with a BipBufferHeader. It provides a method to reserve a contiguous
 * block of memory in the buffer, represented as a BipBufferWriterReservation.
 * The reservation is committed when the unique_ptr is reset or destroyed.
 */
class BipBufferWriter {
public:
  /// Construct a BipBufferWriter as the exclusive writer for a bip buffer
  explicit BipBufferWriter(BipBufferHeader& layout) : layout_(layout) {}

  ~BipBufferWriter() = default;

  BipBufferWriter(const BipBufferWriter&) = delete;
  BipBufferWriter& operator=(const BipBufferWriter&) = delete;
  BipBufferWriter(BipBufferWriter&&) = default;
  BipBufferWriter& operator=(BipBufferWriter&&) = delete;

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
  BipBufferHeader& layout_;

  friend class BipBufferWriterReservation;

  // Commits previously reserved space: updates the `last` and `write` positions
  // in the layout header.
  void commit(size_t start, size_t len, bool wraparound);
};

} // namespace mvi
