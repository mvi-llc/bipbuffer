#include "BipBufferHeader.hpp"

#include <catch2/catch_all.hpp>

#include <array>

TEST_CASE("BipBufferHeader Create", "[bipbuffer]") {
  STATIC_REQUIRE(sizeof(mvi::BipBufferHeader) == 32);

  const size_t bufferSize = 64;
  std::array<uint8_t, bufferSize> buffer{};

  auto layout = mvi::BipBufferHeader::Create(buffer.data(), buffer.size());

  REQUIRE(layout != nullptr);
  CHECK(layout->bufferSize == bufferSize - sizeof(mvi::BipBufferHeader));

  constexpr size_t READ_POS = 1;
  constexpr size_t WRITE_POS = 512;
  constexpr size_t LAST_POS = 1024;

  layout->read = READ_POS;
  layout->write = WRITE_POS;
  layout->last = LAST_POS;

  CHECK(layout->read.load(std::memory_order_seq_cst) == READ_POS);
  CHECK(layout->write.load(std::memory_order_seq_cst) == WRITE_POS);
  CHECK(layout->last.load(std::memory_order_seq_cst) == LAST_POS);
}
