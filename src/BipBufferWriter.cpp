#include "BipBufferWriter.hpp"

namespace mvi {

static size_t SaturatingSub(size_t x, size_t y) {
  size_t res = x - y;
  res &= -(res <= x);
  return res;
}

std::unique_ptr<BipBufferWriterReservation> BipBufferWriter::reserve(size_t length) {
  // First, determine whether there is enough space to reserve `length` bytes
  const size_t currentWrite = layout_.write.load(std::memory_order_seq_cst);
  const size_t currentRead = layout_.read.load(std::memory_order_seq_cst);

  size_t start;
  bool wraparound = false;
  if (currentWrite >= currentRead) {
    // Case 1: There is space from write to the end or from start to read
    // [R.........W------------------------] or
    // [---------------------------R....W--]
    size_t endSpace = SaturatingSub(layout_.bufferSize, currentWrite);
    if (endSpace >= length) {
      start = currentWrite; // Start writing at `currentWrite`
    } else {
      if (SaturatingSub(currentRead, 1) >= length) {
        start = 0; // Start writing at the beginning of the buffer
        wraparound = true;
      } else {
        return nullptr; // Not enough space
      }
    }
  } else {
    // Case 2: There is space from write to read
    // [....W--------------R................]
    // Ensure there's a gap of at least one byte to differentiate from a full buffer
    if (SaturatingSub(currentRead - currentWrite, 1) >= length) {
      start = currentWrite;
    } else {
      return nullptr; // Not enough space
    }
  }

  // Reserve the space (note: actual commit happens when the reservation goes
  // out of scope)
  return std::make_unique<BipBufferWriterReservation>(*this, start, length, wraparound);
}

void BipBufferWriter::commit(size_t start, size_t length, bool wraparound) {
  if (length == 0) { return; }

  const size_t currentWrite = layout_.write.load(std::memory_order_seq_cst);
  const size_t newWrite = start + length;

  // Commit the reserved space: update the `last` and `write` positions

  if (wraparound) {
    // If the reservation involved a wraparound, update `last` to point to the
    // end of the last committed write
    layout_.last.store(currentWrite, std::memory_order_seq_cst);
  } else {
    // No wraparound, only update `last` if the new `write` position extends
    // beyond it
    const size_t currentLast = layout_.last.load(std::memory_order_seq_cst);
    if (newWrite > currentLast) { layout_.last.store(newWrite, std::memory_order_seq_cst); }
  }

  layout_.write.store(newWrite, std::memory_order_seq_cst);
}

} // namespace mvi
