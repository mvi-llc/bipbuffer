#include "BipBufferReader.hpp"
#include "BipBufferWriter.hpp"

#define CATCH_CONFIG_RUNNER
#include <catch2/catch_all.hpp>

#include <array>
#include <thread>
#include <vector>

TEST_CASE("BipBuffer multi-threaded benchmark", "[bipbuffer][concurrent][benchmark]") {
  constexpr size_t BUFFER_SIZE = 128;
  std::array<uint8_t, BUFFER_SIZE> buffer{};

  auto layout = mvi::BipBufferHeader::Create(buffer.data(), buffer.size());
  REQUIRE(layout->bufferSize == BUFFER_SIZE - sizeof(mvi::BipBufferHeader));

  std::unique_ptr<mvi::BipBufferWriter> writer;
  std::unique_ptr<mvi::BipBufferReader> reader;

  std::vector<uint8_t> testData(1024 * 1024 * 10); // 10MB of test data for writing
  for (size_t i = 0; i < testData.size(); ++i) {
    testData[i] = static_cast<uint8_t>(i % 256); // Fill with some pattern
  }

  // Writer thread function
  auto writerFunc = [&]() {
    for (size_t offset = 0; offset < testData.size(); offset += 32) {
      // Try to reserve space and write data in chunks
      auto reservation = writer->reserve(32);
      while (!reservation) {
        std::this_thread::yield(); // Yield thread if reservation failed, then try again
        reservation = writer->reserve(32);
      }
      // std::memcpy(reservation->data(), testData.data() + offset, 32);
      reservation.reset(); // Automatically commits the data
    }
  };

  // Reader thread function
  auto readerFunc = [&]() {
    size_t totalRead = 0;
    while (totalRead < testData.size()) {
      auto data = reader->read();
      if (!data.empty()) {
        // Validate the data read matches expected test data
        totalRead += data.size();
        (void)reader->advance(data.size());
      } else {
        std::this_thread::yield(); // Yield thread if no data available, then try again
      }
    }
  };

  BENCHMARK_ADVANCED("write and read")(Catch::Benchmark::Chronometer meter) {
    // Reset everything before each run
    layout->read.store(0, std::memory_order_seq_cst);
    layout->write.store(0, std::memory_order_seq_cst);
    layout->last.store(0, std::memory_order_seq_cst);
    writer = std::make_unique<mvi::BipBufferWriter>(*layout);
    reader = std::make_unique<mvi::BipBufferReader>(*layout);

    meter.measure([&writerFunc, &readerFunc] {
      // Start writer and reader threads
      std::thread writerThread(writerFunc);
      std::thread readerThread(readerFunc);

      // Wait for both threads to complete
      writerThread.join();
      readerThread.join();
    });
  };
}

int main(int argc, char* argv[]) {
  return Catch::Session().run(argc, argv);
}
