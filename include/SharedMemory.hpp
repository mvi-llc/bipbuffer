#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <system_error>

namespace mvi {

/**
 * A cross-platform abstraction for shared memory. This class provides a simple interface for
 * creating, opening, and destroying shared memory areas. The shared memory area is backed by a
 * file descriptor on Unix-like systems and a named memory-mapped file on Windows.
 */
class SharedMemory {
public:
  enum class Access { ReadOnly, ReadWrite };

  /**
   * Construct a SharedMemory object with the given name and size. The name must be unique, contain
   * only alpha-numeric characters, and be fewer than 256 characters. The size is the number of
   * bytes to allocate for the shared memory area.
   *
   * @param name the name of the shared memory area
   * @param size the size of the shared memory area in bytes
   */
  SharedMemory(const std::string& name, size_t size);

  /// Close the shared memory area if it is still open on destruction
  ~SharedMemory();

  SharedMemory(const SharedMemory&) = default;
  SharedMemory& operator=(const SharedMemory&) = default;
  SharedMemory(SharedMemory&&) = default;
  SharedMemory& operator=(SharedMemory&&) = default;

  /**
   * Open the shared memory area for reading or writing. If the shared memory area does not exist
   * and the access is ReadWrite, it will be created.
   *
   * @param access the access mode, ReadOnly or ReadWrite
   * @return std::nullopt if the operation was successful, otherwise a std::system_error
   */
  std::optional<std::system_error> open(Access access);

  /// Returns the name of the shared memory area, set during construction
  const std::string& name() const;

  /// Returns the size of the shared memory area, set during construction
  size_t size() const;

  /// Returns the total capacity in bytes of the shared memory area. This will be greater than or
  /// equal to the size of the shared memory area
  size_t capacity() const;

  /// Returns a pointer to the start of the shared memory area
  template<typename T> T* as() { return static_cast<T*>(data_); }

  /// Returns a const pointer to the start of the shared memory area
  template<typename T> const T* as() const { return static_cast<const T*>(data_); }

  /// Closes the shared memory area
  std::optional<std::system_error> close();

  /// Static method to destroy a shared memory area by name. Succeeds if the shared memory area is
  /// successfully destroyed or does not exist
  static std::optional<std::system_error> Destroy(const std::string& name);

private:
  std::string name_;
  std::string normalizedName_;
  void* data_ = nullptr;
  size_t size_ = 0;
  size_t capacity_ = 0;
#ifdef _WIN32
  using HANDLE = void*;
  HANDLE handle_;
#else
  int fd_ = -1;
#endif // _WIN32

  std::optional<std::system_error> createOrOpen(bool create);
};

} // namespace mvi
