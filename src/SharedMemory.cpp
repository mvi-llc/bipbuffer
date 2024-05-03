#include "SharedMemory.hpp"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
#include <io.h> // CreateFileMappingA(), OpenFileMappingA(), etc.
#else
#include <errno.h> // errno
#include <fcntl.h> // for O_* constants
#include <limits.h> // IWYU pragma: keep, NAME_MAX
#include <sys/mman.h> // ::mmap(), ::munmap()
#include <sys/stat.h> // for mode constants
#include <unistd.h> // ::close()
#endif // _WIN32

namespace mvi {

SharedMemory::SharedMemory(const std::string& name, const size_t size)
  : name_(name),
    normalizedName_("/" + name),
    size_(size) {}

SharedMemory::~SharedMemory() {
  close();
}

const std::string& SharedMemory::name() const {
  return name_;
}

size_t SharedMemory::size() const {
  return size_;
};

size_t SharedMemory::capacity() const {
  return capacity_;
}

#ifdef _WIN32
// Windows shared memory implementation

std::optional<std::system_error> SharedMemory::open(SharedMemory::Access access) {
  if (name_.empty() || name_.size() > 255) {
    return std::system_error(
      EINVAL, std::system_category(), "name must be between 1 and 255 characters");
  }
  for (const char c : name_) {
    if (!std::isalnum(c)) {
      return std::system_error(
        EINVAL, std::system_category(), "name must only contain alpha-numeric characters");
    }
  }

  if (access == Access::ReadWrite) {
    handle_ = CreateFileMappingA(INVALID_HANDLE_VALUE, // Use the paging file
      nullptr, // Default security
      PAGE_READWRITE, // Read/write access
      0, // High-order DWORD(size_)
      DWORD(size_), // Low-order DWORD(size_)
      name_.c_str()); // Name of mapping object
  } else {
    handle_ = OpenFileMappingA(FILE_MAP_READ, // Read access
      FALSE, // Do not inherit the name
      name_.c_str()); // Name of mapping object
  }
  if (!handle_) {
    const DWORD err = GetLastError();
    return std::system_error(int(err), std::system_category(), "CreateFileMappingA");
  }

  capacity_ = size_;

  const DWORD accessFlags = access == Access::ReadWrite ? FILE_MAP_ALL_ACCESS : FILE_MAP_READ;
  data_ = MapViewOfFile(handle_, accessFlags, 0, 0, size_);

  if (!data_) {
    const DWORD err = GetLastError();
    close();
    return std::system_error(int(err), std::system_category(), "MapViewOfFile");
  }
  return {};
}

std::optional<std::system_error> SharedMemory::close() {
  const void* data = data_;
  HANDLE handle = handle_;
  data_ = nullptr;
  handle_ = nullptr;

  if (data) {
    if (!UnmapViewOfFile(data)) {
      const DWORD err = GetLastError();
      if (handle) { CloseHandle(handle); }
      return std::system_error(int(err), std::system_category(), "UnmapViewOfFile");
    }
  }

  if (handle) {
    if (!CloseHandle(handle)) {
      const DWORD err = GetLastError();
      return std::system_error(int(err), std::system_category(), "CloseHandle");
    }
  }

  return {};
}

std::optional<std::system_error> SharedMemory::Destroy(const std::string& name) {
  if (name.empty()) {
    return std::system_error(
      ERROR_PATH_NOT_FOUND, std::system_category(), "name must not be empty");
  }

  // This is a no-op on Windows, the shared memory area is automatically
  // destroyed when the last handle is closed
  return {};
}

#else
// POSIX shared memory implementation

std::optional<std::system_error> SharedMemory::open(SharedMemory::Access access) {
  if (name_.empty() || name_.size() > NAME_MAX) {
    return std::system_error(EINVAL,
      std::system_category(),
      "name must be between 1 and " + std::to_string(NAME_MAX) + " characters");
  }
  for (const char c : name_) {
    if (!std::isalnum(c)) {
      return std::system_error(
        EINVAL, std::system_category(), "name must only contain alpha-numeric characters");
    }
  }

  const std::string normalizedName = "/" + name_;
  const int flags = access == Access::ReadWrite ? (O_CREAT | O_RDWR) : O_RDONLY;
  fd_ = ::shm_open(normalizedName.c_str(), // NOLINT(cppcoreguidelines-pro-type-vararg)
    flags,
    S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
  if (fd_ < 0) { return std::system_error(errno, std::system_category(), "shm_open"); }

  struct stat shm_stat = {};
  if (::fstat(fd_, &shm_stat) == -1 || shm_stat.st_size < 0) {
    ::close(fd_);
    fd_ = -1;
    return std::system_error(errno, std::system_category(), "fstat");
  }

  capacity_ = size_t(shm_stat.st_size);

  // Ensure the shared memory area size is at least as large as requested
  if (capacity_ < size_) {
    if (access == Access::ReadWrite) {
      // If the file already exists but is too small, return an error
      if (capacity_ > 0) {
        ::close(fd_);
        fd_ = -1;
        return std::system_error(EOVERFLOW, std::system_category(), "not enough capacity");
      }

      // This is the only way to specify the size of a POSIX shared memory object
      if (::ftruncate(fd_, off_t(size_)) == -1) {
        ::close(fd_);
        fd_ = -1;
        return std::system_error(errno, std::system_category(), "ftruncate");
      }

      // Get the updated size after ftruncate
      if (::fstat(fd_, &shm_stat) == -1 || shm_stat.st_size < 0) {
        ::close(fd_);
        fd_ = -1;
        return std::system_error(errno, std::system_category(), "fstat");
      }
      capacity_ = size_t(shm_stat.st_size);

      if (capacity_ < size_) {
        ::close(fd_);
        fd_ = -1;
        return std::system_error(
          ENOMEM, std::system_category(), "not enough capacity after ftruncate");
      }
    } else {
      ::close(fd_);
      fd_ = -1;
      return std::system_error(EEXIST, std::system_category(), "size mismatch");
    }
  }

  const int protections = access == Access::ReadWrite ? (PROT_READ | PROT_WRITE) : PROT_READ;
  data_ = ::mmap(nullptr, // addr
    size_, // length
    protections, // prot
    MAP_SHARED, // flags
    fd_, // fd
    0 // offset
  );
  if (data_ == MAP_FAILED) { return std::system_error(errno, std::system_category(), "mmap"); }

  if (!data_) {
    const int err = errno ? errno : EINVAL;
    this->close();
    return std::system_error(err, std::system_category(), "mmap");
  }
  return {};
}

std::optional<std::system_error> SharedMemory::close() {
  void* data = data_;
  const int fd = fd_;
  data_ = nullptr;
  fd_ = -1;

  if (data && 0 != ::munmap(data, size_)) {
    if (fd) { ::close(fd); }
    return std::system_error(errno, std::system_category(), "munmap");
  }
  if (fd != -1 && ::close(fd) != 0) {
    return std::system_error(errno, std::system_category(), "close");
  }
  return {};
}

std::optional<std::system_error> SharedMemory::Destroy(const std::string& name) {
  if (name.empty()) {
    return std::system_error(EINVAL, std::system_category(), "name must not be empty");
  }
  const std::string normalizedName = name.front() == '/' ? name : "/" + name;

  if (::shm_unlink(normalizedName.c_str()) != 0) {
    return std::system_error(errno, std::system_category(), "shm_unlink");
  }
  return {};
}

#endif // _WIN32

} // namespace mvi
