#include "BipBufferMemoryLayout.hpp"

#include <new> // IWYU pragma: keep (placement new)

namespace mvi {

BipBufferMemoryLayout* BipBufferMemoryLayout::Create(uint8_t* data, size_t size) {
  if (!data || size <= sizeof(BipBufferMemoryLayout)) { return nullptr; }
  // Explicitly using a raw pointer to indicate non-ownership
  auto layout = new (data) BipBufferMemoryLayout(); // NOLINT(cppcoreguidelines-owning-memory)
  layout->read.store(0, std::memory_order_seq_cst);
  layout->write.store(0, std::memory_order_seq_cst);
  layout->last.store(0, std::memory_order_seq_cst);
  layout->bufferSize = size - sizeof(BipBufferMemoryLayout);
  return layout;
}

} // namespace mvi
