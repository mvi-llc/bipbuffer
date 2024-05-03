#include "BipBufferReader.hpp"

#include <catch2/catch_all.hpp>

#include <array>
#include <cstring> // for memcpy

constexpr auto ORDER_STRICT = std::memory_order_seq_cst;

TEST_CASE("BipBufferReader basic lifecycle", "[bipbuffer]") {
  constexpr size_t BUFFER_SIZE = 64;

  std::array<uint8_t, 256> testData;
  for (size_t i = 0; i < testData.size(); i++) {
    testData[i] = uint8_t(i);
  }

  std::array<uint8_t, BUFFER_SIZE> buffer{};
  auto layout = mvi::BipBufferMemoryLayout::Create(buffer.data(), buffer.size());
  REQUIRE(layout->bufferSize == BUFFER_SIZE - sizeof(mvi::BipBufferMemoryLayout));

  mvi::BipBufferReader reader{*layout};

  // Initially, there should be nothing to read
  std::string_view data = reader.read();
  REQUIRE(data.empty());

  // Simulate writing some data
  std::memcpy(layout->buffer, testData.data(), 5);
  layout->write.store(5, ORDER_STRICT);

  // Now there should be something to read
  data = reader.read();
  REQUIRE(data.size() == 5);
  REQUIRE(std::memcmp(data.data(), testData.data(), data.size()) == 0);

  // Consume the data
  REQUIRE(reader.advance(data.size()) == true);
  REQUIRE(layout->read.load(ORDER_STRICT) == 5);

  // After consuming, there should be nothing to read
  data = reader.read();
  REQUIRE(data.empty());

  // The read position should have updated
  size_t currentRead = layout->read.load(ORDER_STRICT);
  REQUIRE(currentRead == 5);

  // Write to the end of the buffer
  std::memcpy(layout->buffer + 5, testData.data() + 5, 27);
  layout->last.store(32, ORDER_STRICT);
  layout->write.store(32, ORDER_STRICT);

  // Now there should be something to read
  data = reader.read();
  REQUIRE(data.size() == 27);
  REQUIRE(std::memcmp(data.data(), testData.data() + 5, data.size()) == 0);
  reader.advance(27);
  REQUIRE(layout->read.load(ORDER_STRICT) == 32);

  // Simulate writing after wraparound
  std::memcpy(layout->buffer, testData.data() + 32, 1);
  layout->write.store(1, ORDER_STRICT);

  // Now there should be something to read
  data = reader.read();
  REQUIRE(data.size() == 1);
  REQUIRE(data.data()[0] == testData.data()[32]);
  reader.advance(1);
  REQUIRE(layout->read.load(ORDER_STRICT) == 1);

  // Confirm there is nothing to read
  data = reader.read();
  REQUIRE(data.empty());

  // Simulate writing 30 bytes
  std::memcpy(layout->buffer + 1, testData.data() + 33, 30);
  layout->last.store(31, ORDER_STRICT);
  layout->write.store(31, ORDER_STRICT);

  // Now there should be something to read
  data = reader.read();
  REQUIRE(data.size() == 30);
  REQUIRE(std::memcmp(data.data(), testData.data() + 33, data.size()) == 0);
  reader.advance(data.size());
  REQUIRE(layout->read.load(ORDER_STRICT) == 31);

  // Simulate another wraparound, where `last` is less than the buffer size
  std::memcpy(layout->buffer, testData.data() + 63, 2);
  layout->write.store(2, ORDER_STRICT);

  // Now there should be something to read
  data = reader.read();
  REQUIRE(data.size() == 2);
  REQUIRE(std::memcmp(data.data(), testData.data() + 63, data.size()) == 0);
  reader.advance(data.size());
  REQUIRE(layout->read.load(ORDER_STRICT) == 2);
}
