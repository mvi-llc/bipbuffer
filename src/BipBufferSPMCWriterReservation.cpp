#include "BipBufferSPMCWriterReservation.hpp"

#include "BipBufferSPMCWriter.hpp"

namespace mvi {

BipBufferSPMCWriterReservation::BipBufferSPMCWriterReservation(
  BipBufferSPMCWriter& writer, size_t start, size_t len, bool wraparound)
  : writer_(writer),
    start_(start),
    length_(len),
    wraparound_(wraparound) {}

BipBufferSPMCWriterReservation::~BipBufferSPMCWriterReservation() {
  // If the reservation has not been canceled (len_ > 0), update the buffer's
  // write index to commit this reservation
  if (length_ > 0) { writer_.commit(start_, length_, wraparound_); }
}

uint8_t* BipBufferSPMCWriterReservation::data() {
  return writer_.layout_.buffer() + start_;
}

size_t BipBufferSPMCWriterReservation::size() const {
  return length_;
}

bool BipBufferSPMCWriterReservation::truncate(size_t newSize) {
  if (newSize > length_) { return false; }
  length_ = newSize;
  return true;
}

void BipBufferSPMCWriterReservation::cancel() {
  // Effectively "deletes" this reservation by setting its length to zero
  length_ = 0;
}

} // namespace mvi
