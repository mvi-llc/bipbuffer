#include "BipBufferHeader.hpp"

#include <new> // IWYU pragma: keep (placement new)

namespace mvi {

const uint8_t* BipBufferHeader::buffer() const {
  return reinterpret_cast<const uint8_t*>(this) + sizeof(BipBufferHeader);
}

uint8_t* BipBufferHeader::buffer() {
  return reinterpret_cast<uint8_t*>(this) + sizeof(BipBufferHeader);
}

BipBufferHeader* BipBufferHeader::Create(uint8_t* data, size_t size) {
  if (!data || size <= sizeof(BipBufferHeader)) { return nullptr; }
  // Explicitly using a raw pointer to indicate non-ownership
  auto layout = new (data) BipBufferHeader(); // NOLINT(cppcoreguidelines-owning-memory)
  layout->read = 0;
  layout->write = 0;
  layout->last = 0;
  layout->bufferSize = uint64_t(size - sizeof(BipBufferHeader));
  return layout;
}

} // namespace mvi
