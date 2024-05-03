#include "BipBufferWriterReservation.hpp"

#include "BipBufferWriter.hpp"

namespace mvi {

BipBufferWriterReservation::BipBufferWriterReservation(
  BipBufferWriter& writer, size_t start, size_t len, bool wraparound)
  : writer_(writer),
    start_(start),
    length_(len),
    wraparound_(wraparound) {}

BipBufferWriterReservation::~BipBufferWriterReservation() {
  // If the reservation has not been canceled (len_ > 0), update the buffer's
  // write index to commit this reservation
  if (length_ > 0) { writer_.commit(start_, length_, wraparound_); }
}

uint8_t* BipBufferWriterReservation::data() {
  return writer_.data() + start_;
}

size_t BipBufferWriterReservation::size() const {
  return length_;
}

bool BipBufferWriterReservation::truncate(size_t newSize) {
  if (newSize > length_) { return false; }
  length_ = newSize;
  return true;
}

void BipBufferWriterReservation::cancel() {
  // Effectively "deletes" this reservation by setting its length to zero
  length_ = 0;
}

} // namespace mvi
