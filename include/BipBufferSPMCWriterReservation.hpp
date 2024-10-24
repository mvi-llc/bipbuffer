#pragma once

#include <cstddef>
#include <cstdint>

namespace mvi {

class BipBufferSPMCWriter;

class BipBufferSPMCWriterReservation {
public:
  /**
   * Construct a BipBufferSPMCWriterReservation for a contiguous block of memory
   * in a BipBuffer. The reservation is committed when the reservation object is
   * destroyed.
   */
  BipBufferSPMCWriterReservation(
    BipBufferSPMCWriter& writer, size_t start, size_t length, bool wraparound);

  /// The reservation is committed on destruction
  ~BipBufferSPMCWriterReservation();

  // No copying or moving allowed to prevent issues
  BipBufferSPMCWriterReservation(const BipBufferSPMCWriterReservation&) = delete;
  BipBufferSPMCWriterReservation& operator=(const BipBufferSPMCWriterReservation&) = delete;
  BipBufferSPMCWriterReservation(BipBufferSPMCWriterReservation&&) = delete;
  BipBufferSPMCWriterReservation& operator=(BipBufferSPMCWriterReservation&&) = delete;

  /// Access the reserved buffer slice for writing
  uint8_t* data();

  /// Returns the size of the reserved buffer slice
  size_t size() const;

  /**
   * Truncate the reservation to a smaller size. This is useful when the
   * reservation is made before the final message length is known: the sender
   * can reserve a length that is an upper-bound of the expected message size,
   * write the message to the reservation buffer, and then truncate the
   * reservation to the actual message length written.
   *
   * @param newSize The new size to truncate the reservation to. Must be <= the
   *   current reservation size.
   * @return True if truncation succeeded, false if newSize is invalid.
   */
  [[nodiscard]] bool truncate(size_t newSize);

  /// Cancel the reservation by truncating it to zero-length
  void cancel();

private:
  BipBufferSPMCWriter& writer_; // Reference to the writer to notify when sending
  size_t start_; // Start of the reserved buffer slice
  size_t length_; // Length of the reserved buffer slice
  bool wraparound_; // Does the reservation wrap around the end of the buffer
};

} // namespace mvi
