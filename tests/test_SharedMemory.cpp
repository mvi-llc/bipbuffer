#include "SharedMemory.hpp"

#include <catch2/catch_all.hpp>

#include <array>

using Access = mvi::SharedMemory::Access;

TEST_CASE("SharedMemory basic lifecycle", "[shm]") {
  constexpr const char* NAME = "test";
  constexpr size_t SIZE = 1024;

  // Reset to a known state
  mvi::SharedMemory::Destroy(NAME);

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
  REQUIRE_FALSE(err);
  CHECK(shmWriter.name() == NAME);
  CHECK(shmWriter.size() == SIZE);
  CHECK(shmWriter.capacity() >= SIZE);
  CHECK(shmWriter.as<char>() != nullptr);

  err = shmReader.open(Access::ReadOnly);
  REQUIRE_FALSE(err);
  CHECK(shmReader.name() == NAME);
  CHECK(shmReader.size() == SIZE);
  CHECK(shmReader.capacity() >= SIZE);
  CHECK(shmReader.as<char>() != nullptr);

  // Write to shmWriter, read from shmReader
  std::array<char, SIZE> data;
  std::fill(data.begin(), data.end(), 'A');

  std::copy(data.begin(), data.end(), shmWriter.as<char>());
  CHECK(std::equal(data.begin(), data.end(), shmReader.as<char>()));

  err = shmWriter.close();
  REQUIRE_FALSE(err);
  err = shmReader.close();
  REQUIRE_FALSE(err);

  // Destroy the shared memory
  err = mvi::SharedMemory::Destroy(NAME);
  REQUIRE_FALSE(err);
}

TEST_CASE("SharedMemory increasing sizes", "[shm]") {
  constexpr const char* NAME = "sizes";

  // Reset to a known state
  mvi::SharedMemory::Destroy(NAME);

  // Create shms with powers of 2 from 1 to 16MB
  for (size_t size = 1; size < 16 * 1024 * 1024; size *= 2) {
    mvi::SharedMemory::Destroy(NAME);

    mvi::SharedMemory shm(NAME, size);
    auto err = shm.open(Access::ReadWrite);
    REQUIRE_FALSE(err);
    CHECK(shm.size() == size);
    CHECK(shm.capacity() >= size);
    err = shm.close();
    REQUIRE_FALSE(err);
  }

  // Destroy the shared memory
  auto err = mvi::SharedMemory::Destroy(NAME);
  REQUIRE_FALSE(err);
}

TEST_CASE("SharedMemory decreasing size", "[shm]") {
  constexpr const char* NAME = "decreasing";

  // Reset to a known state
  mvi::SharedMemory::Destroy(NAME);

  // Create shm with size 8 and write to the first four bytes
  mvi::SharedMemory shm(NAME, 8);
  auto err = shm.open(Access::ReadWrite);
  REQUIRE_FALSE(err);
  std::array<char, 4> data = {'A', 'B', 'C', 'D'};
  std::copy(data.begin(), data.end(), shm.as<char>());
  err = shm.close();
  REQUIRE_FALSE(err);

  // Open the shm with size 4 and read from it
  shm = mvi::SharedMemory(NAME, 4);
  err = shm.open(Access::ReadOnly);
  REQUIRE_FALSE(err);
  CHECK(std::equal(data.begin(), data.end(), shm.as<char>()));

  // Destroy the shared memory
  err = mvi::SharedMemory::Destroy(NAME);
  REQUIRE_FALSE(err);
}

TEST_CASE("SharedMemory out of order operations", "[shm]") {
  constexpr const char* NAME = "outoforder";
  constexpr size_t SIZE = 1024;

  // Reset to a known state
  mvi::SharedMemory::Destroy(NAME);

  // Destroying non-existent shm should error
  auto err = mvi::SharedMemory::Destroy(NAME);
  REQUIRE(err);

  // Closing without opening should succeed
  mvi::SharedMemory shm(NAME, SIZE);
  err = shm.close();
  REQUIRE_FALSE(err);

  // Opening read-only without existing shm should fail
  shm = mvi::SharedMemory(NAME, SIZE);
  err = shm.open(Access::ReadOnly);
  REQUIRE(err);
}

TEST_CASE("SharedMemory invalid usage", "[shm]") {
  const auto access = GENERATE(Access::ReadOnly, Access::ReadWrite);

  // leading slash
  mvi::SharedMemory shm("/invalid", 1024);
  auto err = shm.open(access);
  CHECK(err);

  // non-alphanumeric
  shm = mvi::SharedMemory("not valid", 1024);
  err = shm.open(access);
  CHECK(err);

  // more than 255 characters
  std::string longName(256, 'x');
  shm = mvi::SharedMemory(longName, 1024);
  err = shm.open(access);
  CHECK(err);

  // Valid name, invalid size
  mvi::SharedMemory::Destroy("valid");
  shm = mvi::SharedMemory("valid", 0);
  err = shm.open(access);
  CHECK(err);
}

TEST_CASE("SharedMemory persistence", "[shm]") {
  constexpr const char* NAME = "persist";
  constexpr size_t SIZE = 64;

  // Reset to a known state
  mvi::SharedMemory::Destroy(NAME);

  mvi::SharedMemory shm(NAME, SIZE);
  auto err = shm.open(Access::ReadWrite);
  REQUIRE_FALSE(err);

  CHECK(shm.name() == NAME);
  CHECK(shm.size() == SIZE);
  CHECK(shm.capacity() >= SIZE);
  CHECK(shm.as<char>() != nullptr);

  // Write to shm, close it, open it again, read from it

  std::array<char, SIZE> data;
  std::fill(data.begin(), data.end(), 'B');
  std::copy(data.begin(), data.end(), shm.as<char>());
  CHECK(std::equal(data.begin(), data.end(), shm.as<char>()));

  err = shm.close();
  REQUIRE_FALSE(err);

  shm = mvi::SharedMemory(NAME, SIZE);
  err = shm.open(Access::ReadWrite);
  REQUIRE_FALSE(err);

  CHECK(std::equal(data.begin(), data.end(), shm.as<char>()));

  err = shm.close();
  REQUIRE_FALSE(err);

  // Create a shm with the same name and size, it should succeed
  shm = mvi::SharedMemory(NAME, SIZE);
  err = shm.open(Access::ReadWrite);
  REQUIRE_FALSE(err);

  // Create a shm with the same name and considerably larger size, it should fail
  shm = mvi::SharedMemory(NAME, SIZE + 1024 * 1024);
  err = shm.open(Access::ReadWrite);
  CHECK(err);

  // Destroy the shared memory
  err = mvi::SharedMemory::Destroy(NAME);
  REQUIRE_FALSE(err);
}
