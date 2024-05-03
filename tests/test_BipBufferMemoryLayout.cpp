#include "BipBufferMemoryLayout.hpp"

#include <catch2/catch_all.hpp>

#include <array>

TEST_CASE("BipBufferMemoryLayout Create", "[bipbuffer]") {
  STATIC_REQUIRE(sizeof(mvi::BipBufferMemoryLayout) == 32);

  const size_t bufferSize = 64;
  std::array<uint8_t, bufferSize> buffer;

  auto layout = mvi::BipBufferMemoryLayout::Create(buffer.data(), buffer.size());

  REQUIRE(layout != nullptr);
  CHECK(layout->bufferSize == bufferSize - sizeof(mvi::BipBufferMemoryLayout));

  layout->read = 1;
  layout->write = 512;
  layout->last = 1024;

  CHECK(layout->read.load(std::memory_order_seq_cst) == 1);
  CHECK(layout->write.load(std::memory_order_seq_cst) == 512);
  CHECK(layout->last.load(std::memory_order_seq_cst) == 1024);
}
