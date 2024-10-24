#include "BipBufferReader.hpp"
#include "BipBufferWriter.hpp"

#include <catch2/catch_all.hpp>

#include <array>
#include <thread>
#include <vector>

TEST_CASE("BipBuffer concurrent access", "[bipbuffer][concurrent]") {
  constexpr size_t BUFFER_SIZE = 128;
  std::array<uint8_t, BUFFER_SIZE> buffer{};

  auto layout = mvi::BipBufferHeader::Create(buffer.data(), buffer.size());
  REQUIRE(layout->bufferSize == BUFFER_SIZE - sizeof(mvi::BipBufferHeader));

  mvi::BipBufferWriter writer{*layout};
  mvi::BipBufferReader reader{*layout};

  std::vector<uint8_t> testData(1024 * 1024 * 10); // 10MB of test data for writing
  for (size_t i = 0; i < testData.size(); ++i) {
    testData[i] = static_cast<uint8_t>(i % 256); // Fill with some pattern
  }

  // Writer thread function
  auto writerFunc = [&]() {
    for (size_t offset = 0; offset < testData.size(); offset += 32) {
      // Try to reserve space and write data in chunks
      auto reservation = writer.reserve(32);
      while (!reservation) {
        std::this_thread::yield(); // Yield thread if reservation failed, then try again
        reservation = writer.reserve(32);
      }
      std::memcpy(reservation->data(), testData.data() + offset, 32);
      reservation.reset(); // Automatically commits the data
    }
  };

  // Reader thread function
  auto readerFunc = [&]() {
    size_t totalRead = 0;
    while (totalRead < testData.size()) {
      auto data = reader.read();
      if (!data.empty()) {
        // Validate the data read matches expected test data
        REQUIRE(data.size() <= layout->bufferSize);
        REQUIRE(std::memcmp(data.data(), testData.data() + totalRead, data.size()) == 0);
        totalRead += data.size();
        REQUIRE(reader.advance(data.size()) == true);
      } else {
        std::this_thread::yield(); // Yield thread if no data available, then try again
      }
    }
  };

  // Start writer and reader threads
  std::thread writerThread(writerFunc);
  std::thread readerThread(readerFunc);

  // Wait for both threads to complete
  writerThread.join();
  readerThread.join();
}
