#include "BipBufferReader.hpp"

namespace mvi {

BipBufferReader::BipBufferReader(BipBufferMemoryLayout& layout)
  : layout_(layout),
    cachedRead_(layout.read.load(std::memory_order_seq_cst)),
    cachedWrite_(layout.write.load(std::memory_order_seq_cst)),
    cachedLast_(layout.last.load(std::memory_order_seq_cst)) {}

size_t BipBufferReader::offset() const {
  return layout_.read.load(std::memory_order_seq_cst);
}

std::string_view BipBufferReader::read() {
  cachedWrite_ = layout_.write.load(std::memory_order_seq_cst);

  if (cachedWrite_ >= cachedRead_) {
    // No wraparound
    const char* data = reinterpret_cast<const char*>(&layout_.buffer[cachedRead_]);
    return std::string_view{data, cachedWrite_ - cachedRead_};
  } else {
    cachedLast_ = layout_.last.load(std::memory_order_seq_cst);
    if (cachedRead_ == cachedLast_) {
      cachedRead_ = 0;
      return read();
    }

    // Wraparound case
    const char* data = reinterpret_cast<const char*>(&layout_.buffer[cachedRead_]);
    return std::string_view{data, cachedLast_ - cachedRead_};
  }
}

bool BipBufferReader::advance(size_t count) {
  if (cachedWrite_ >= cachedRead_) {
    if (count <= cachedWrite_ - cachedRead_) {
      cachedRead_ += count;
    } else {
      return false;
    }
  } else {
    const size_t remaining = cachedLast_ - cachedRead_;
    if (count == remaining) {
      cachedRead_ = 0;
    } else if (count <= remaining) {
      cachedRead_ += count;
    } else {
      return false;
    }
  }

  layout_.read.store(cachedRead_, std::memory_order_seq_cst);
  return true;
}

} // namespace mvi
