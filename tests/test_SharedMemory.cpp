#include "SharedMemory.hpp"
#include "requires.hpp"

#include <catch2/catch_all.hpp>

#include <array>

using Access = mvi::SharedMemory::Access;

TEST_CASE("SharedMemory basic lifecycle", "[shm]") {
  constexpr const char* NAME = "test";
  constexpr size_t SIZE = 1024;

  // Reset to a known state
  REQUIRE_NO_ERROR(mvi::SharedMemory::Destroy(NAME));

  mvi::SharedMemory shmWriter(NAME, SIZE);
  mvi::SharedMemory shmReader(NAME, SIZE);

  CHECK(shmWriter.name() == NAME);
  CHECK(shmReader.name() == NAME);
  CHECK(shmWriter.size() == SIZE);
  CHECK(shmReader.size() == SIZE);
  CHECK(shmWriter.capacity() == 0);
  CHECK(shmReader.capacity() == 0);
  CHECK(shmWriter.as<char>() == nullptr);
  CHECK(shmReader.as<char>() == nullptr);

  auto err = shmWriter.open(Access::ReadWrite);
  REQUIRE_NO_ERROR(err);
  CHECK(shmWriter.name() == NAME);
  CHECK(shmWriter.size() == SIZE);
  CHECK(shmWriter.capacity() >= SIZE);
  CHECK(shmWriter.as<char>() != nullptr);

  err = shmReader.open(Access::ReadOnly);
  REQUIRE_NO_ERROR(err);
  CHECK(shmReader.name() == NAME);
  CHECK(shmReader.size() == SIZE);
  CHECK(shmReader.capacity() >= SIZE);
  CHECK(shmReader.as<char>() != nullptr);

  // Write to shmWriter, read from shmReader
  std::array<char, SIZE> data{};
  std::fill(data.begin(), data.end(), 'A');

  std::copy(data.begin(), data.end(), shmWriter.as<char>());
  CHECK(std::equal(data.begin(), data.end(), shmReader.as<char>()));

  err = shmWriter.close();
  REQUIRE_NO_ERROR(err);
  err = shmReader.close();
  REQUIRE_NO_ERROR(err);

  // Destroy the shared memory
  err = mvi::SharedMemory::Destroy(NAME);
  REQUIRE_NO_ERROR(err);
}

TEST_CASE("SharedMemory increasing sizes", "[shm]") {
  constexpr const char* NAME = "sizes";

  // Reset to a known state
  REQUIRE_NO_ERROR(mvi::SharedMemory::Destroy(NAME));

  // Create shms with powers of 2 from 1 to 16MB
  constexpr size_t MAX_SIZE = 16 * 1024 * 1024;
  for (size_t size = 1; size < MAX_SIZE; size *= 2) {
    REQUIRE_NO_ERROR(mvi::SharedMemory::Destroy(NAME));

    mvi::SharedMemory shm(NAME, size);
    auto err = shm.open(Access::ReadWrite);
    REQUIRE_NO_ERROR(err);
    CHECK(shm.size() == size);
    CHECK(shm.capacity() >= size);
    err = shm.close();
    REQUIRE_NO_ERROR(err);
  }

  // Destroy the shared memory
  REQUIRE_NO_ERROR(mvi::SharedMemory::Destroy(NAME));
}

TEST_CASE("SharedMemory decreasing size", "[shm]") {
  constexpr const char* NAME = "decreasing";

  // Reset to a known state
  REQUIRE_NO_ERROR(mvi::SharedMemory::Destroy(NAME));

  // Create shm with size 8 and write to the first four bytes
  constexpr size_t EIGHT = 8;
  mvi::SharedMemory shm(NAME, EIGHT);
  auto err = shm.open(Access::ReadWrite);
  REQUIRE_NO_ERROR(err);
  std::array<char, 4> data = {'A', 'B', 'C', 'D'};
  std::copy(data.begin(), data.end(), shm.as<char>());

  // Open the shm with size 4 and read from it
  mvi::SharedMemory shmReader(NAME, 4);
  err = shmReader.open(Access::ReadOnly);
  REQUIRE_NO_ERROR(err);
  CHECK(std::equal(data.begin(), data.end(), shm.as<char>()));

  // Close the shms
  REQUIRE_NO_ERROR(shm.close());
  REQUIRE_NO_ERROR(shmReader.close());

  // Destroy the shared memory
  err = mvi::SharedMemory::Destroy(NAME);
  REQUIRE_NO_ERROR(err);
}

TEST_CASE("SharedMemory out of order operations", "[shm]") {
  constexpr const char* NAME = "outoforder";
  constexpr size_t SIZE = 1024;

  // Reset to a known state
  mvi::SharedMemory::Destroy(NAME);

  // Destroying non-existent shm should succeed
  auto err = mvi::SharedMemory::Destroy(NAME);
  REQUIRE_NO_ERROR(err);

  // Closing without opening should succeed
  mvi::SharedMemory shm(NAME, SIZE);
  err = shm.close();
  REQUIRE_NO_ERROR(err);

  // Opening read-only without existing shm should fail
  shm = mvi::SharedMemory(NAME, SIZE);
  err = shm.open(Access::ReadOnly);
  REQUIRE(err);
}

TEST_CASE("SharedMemory invalid usage", "[shm]") {
  constexpr size_t SIZE = 1024;
  const auto access = GENERATE(Access::ReadOnly, Access::ReadWrite);

  // Leading slash
  mvi::SharedMemory shm("/invalid", SIZE);
  auto err = shm.open(access);
  CHECK(err);

  // Non-alphanumeric
  shm = mvi::SharedMemory("not valid", SIZE);
  err = shm.open(access);
  CHECK(err);

  // More than 255 characters
  constexpr size_t MAX_NAME_SIZE = 255;
  std::string longName(MAX_NAME_SIZE + 1, 'x');
  shm = mvi::SharedMemory(longName, SIZE);
  err = shm.open(access);
  CHECK(err);

  // Valid name, invalid size
  REQUIRE_NO_ERROR(mvi::SharedMemory::Destroy("valid"));
  shm = mvi::SharedMemory("valid", 0);
  err = shm.open(access);
  CHECK(err);

  // Read-only access to non-existent shm
  REQUIRE_NO_ERROR(mvi::SharedMemory::Destroy("nonexistent"));
  shm = mvi::SharedMemory("nonexistent", SIZE);
  err = shm.open(Access::ReadOnly);
  CHECK(err);
}

TEST_CASE("SharedMemory persistence", "[shm]") {
  constexpr const char* NAME = "persist";
  constexpr size_t SIZE = 64;

  // Reset to a known state
  REQUIRE_NO_ERROR(mvi::SharedMemory::Destroy(NAME));

  mvi::SharedMemory shm(NAME, SIZE);
  auto err = shm.open(Access::ReadWrite);
  REQUIRE_NO_ERROR(err);

  CHECK(shm.name() == NAME);
  CHECK(shm.size() == SIZE);
  CHECK(shm.capacity() >= SIZE);
  CHECK(shm.as<char>() != nullptr);

  // Write to shm, close it, open it again, read from it

  std::array<char, SIZE> data{};
  std::fill(data.begin(), data.end(), 'B');
  std::copy(data.begin(), data.end(), shm.as<char>());
  CHECK(std::equal(data.begin(), data.end(), shm.as<char>()));

  err = shm.close();
  REQUIRE_NO_ERROR(err);

  shm = mvi::SharedMemory(NAME, SIZE);
  err = shm.open(Access::ReadWrite);
  REQUIRE_NO_ERROR(err);

  CHECK(std::equal(data.begin(), data.end(), shm.as<char>()));

  err = shm.close();
  REQUIRE_NO_ERROR(err);

  // Create a shm with the same name and size, it should succeed
  shm = mvi::SharedMemory(NAME, SIZE);
  err = shm.open(Access::ReadWrite);
  REQUIRE_NO_ERROR(err);

  // Create a shm with the same name and considerably larger size, it should fail
  constexpr size_t LARGER_SIZE = SIZE + 1024 * 1024;
  shm = mvi::SharedMemory(NAME, LARGER_SIZE);
  err = shm.open(Access::ReadWrite);
  CHECK(err);

  // Destroy the shared memory
  err = mvi::SharedMemory::Destroy(NAME);
  REQUIRE_NO_ERROR(err);
}
