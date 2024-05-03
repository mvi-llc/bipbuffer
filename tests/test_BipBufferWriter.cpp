#include "BipBufferWriter.hpp"

#include <catch2/catch_all.hpp>

#include <array>
#include <cstring> // for memcpy

constexpr auto ORDER_STRICT = std::memory_order_seq_cst;
constexpr size_t HEADER_SIZE = sizeof(mvi::BipBufferMemoryLayout);

TEST_CASE("BipBufferWriter basic lifecycle", "[bipbuffer]") {
  constexpr size_t BUFFER_SIZE = 64;
  std::array<uint8_t, BUFFER_SIZE> buffer;

  auto layout = mvi::BipBufferMemoryLayout::Create(buffer.data(), buffer.size());
  REQUIRE(layout->bufferSize == 32);

  mvi::BipBufferWriter writer{*layout};

  // Can't reserve more than the total buffer size
  auto reservation = writer.reserve(33);
  REQUIRE(reservation == nullptr);

  // Reserve exactly the max available space
  reservation = writer.reserve(32);
  REQUIRE(reservation != nullptr);
  REQUIRE(reservation->size() == 32);
  REQUIRE(reservation->data() == buffer.data() + HEADER_SIZE);

  // Write some data
  uint8_t testData[] = {0x01, 0x02, 0x03};
  memcpy(reservation->data(), testData, sizeof(testData));

  // Confirm the bipbuffer header hasn't changed
  REQUIRE(layout->read.load(ORDER_STRICT) == 0);
  REQUIRE(layout->write.load(ORDER_STRICT) == 0);
  REQUIRE(layout->last.load(ORDER_STRICT) == 0);

  reservation.reset(); // Commit the reservation

  // Confirm the bipbuffer header was updated
  REQUIRE(layout->read.load(ORDER_STRICT) == 0);
  REQUIRE(layout->write.load(ORDER_STRICT) == 32);
  REQUIRE(layout->last.load(ORDER_STRICT) == 32);

  // Check that the data was written
  uint8_t readData[sizeof(testData)];
  memcpy(readData, buffer.data() + HEADER_SIZE, sizeof(readData));
  for (size_t i = 0; i < sizeof(testData); ++i) {
    REQUIRE(readData[i] == testData[i]);
  }

  // Attempt to reserve more data, it will fail since the buffer is full
  reservation = writer.reserve(1);
  REQUIRE(reservation == nullptr);

  // Move the read pointer forward one byte
  layout->read.store(1, ORDER_STRICT);

  reservation = writer.reserve(1);
  REQUIRE(reservation == nullptr);

  // Move the read pointer forward one more byte
  layout->read.store(2, ORDER_STRICT);

  reservation = writer.reserve(1);
  REQUIRE(reservation != nullptr);
  REQUIRE(reservation->size() == 1);
  REQUIRE(reservation->data() == buffer.data() + HEADER_SIZE);

  reservation.reset();

  REQUIRE(layout->read.load(ORDER_STRICT) == 2);
  REQUIRE(layout->write.load(ORDER_STRICT) == 1);
  REQUIRE(layout->last.load(ORDER_STRICT) == 32);

  // Test reservation at a non-zero offset
  layout->write.store(5, ORDER_STRICT);
  layout->read.store(2, ORDER_STRICT);
  reservation = writer.reserve(3);
  REQUIRE(reservation != nullptr);
  REQUIRE(reservation->size() == 3);
  REQUIRE(reservation->data() == buffer.data() + HEADER_SIZE + 5);
  reservation.reset();

  // Test wrapping reservation
  layout->write.store(28);
  layout->read.store(26);
  reservation = writer.reserve(5);
  REQUIRE(reservation != nullptr);
  REQUIRE(reservation->size() == 5);
  REQUIRE(reservation->data() == buffer.data() + HEADER_SIZE);

  REQUIRE(layout->read.load(ORDER_STRICT) == 26);
  REQUIRE(layout->write.load(ORDER_STRICT) == 28);
  REQUIRE(layout->last.load(ORDER_STRICT) == 32);

  reservation.reset();

  REQUIRE(layout->read.load(ORDER_STRICT) == 26);
  REQUIRE(layout->write.load(ORDER_STRICT) == 5);
  REQUIRE(layout->last.load(ORDER_STRICT) == 28);

  // Test truncation
  reservation = writer.reserve(4);
  REQUIRE(reservation != nullptr);
  REQUIRE_FALSE(reservation->truncate(5));
  REQUIRE(reservation->truncate(2));
  reservation.reset();

  REQUIRE(layout->read.load(ORDER_STRICT) == 26);
  REQUIRE(layout->write.load(ORDER_STRICT) == 7);
  REQUIRE(layout->last.load(ORDER_STRICT) == 28);

  // Ensure `last` can be reset to the end of the buffer
  layout->read.store(1, ORDER_STRICT);
  reservation = writer.reserve(25);
  REQUIRE(reservation != nullptr);
  REQUIRE(reservation->size() == 25);
  REQUIRE(reservation->data() == buffer.data() + HEADER_SIZE + 7);
  reservation.reset();

  REQUIRE(layout->read.load(ORDER_STRICT) == 1);
  REQUIRE(layout->write.load(ORDER_STRICT) == 32);
  REQUIRE(layout->last.load(ORDER_STRICT) == 32);

  // Test cancellation
  layout->read.store(31, ORDER_STRICT);
  reservation = writer.reserve(10);
  reservation->cancel();
  reservation.reset();

  REQUIRE(layout->read.load(ORDER_STRICT) == 31);
  REQUIRE(layout->write.load(ORDER_STRICT) == 32);
  REQUIRE(layout->last.load(ORDER_STRICT) == 32);
}
