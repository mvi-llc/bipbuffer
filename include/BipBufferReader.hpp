#pragma once

#include "BipBufferMemoryLayout.hpp"

#include <string_view>

namespace mvi {

/**
 * A BipBufferReader is used to read data from a BipBufferMemoryLayout. It
 * provides methods to read data from the buffer and advance the read position.
 */
class BipBufferReader {
public:
  /// Construct a BipBufferReader as the exclusive reader for a BipBufferMemoryLayout.
  explicit BipBufferReader(BipBufferMemoryLayout& layout);

  ~BipBufferReader() = default;

  BipBufferReader(const BipBufferReader&) = delete;
  BipBufferReader& operator=(const BipBufferReader&) = delete;
  BipBufferReader(BipBufferReader&&) = default;
  BipBufferReader& operator=(BipBufferReader&&) = delete;

  /// Returns the current read offset
  size_t offset() const;

  /**
   * Peeks at the next available bytes in the buffer without advancing the read
   * position. `std::string_view` is used as a zero-copy view for the peeked
   * binary data, it does not necessarily represent the data as a text string.
   * If no new data is available, an empty string_view is returned.
   */
  std::string_view read();

  /**
   * Advances the read position by the given number of bytes. Returns true if
   * the read position was advanced, false if there were insufficient bytes
   * available and the read position was not changed.
   */
  [[nodiscard]] bool advance(size_t count);

private:
  BipBufferMemoryLayout& layout_;
  size_t cachedRead_;
  size_t cachedWrite_;
  size_t cachedLast_;
};

} // namespace mvi
