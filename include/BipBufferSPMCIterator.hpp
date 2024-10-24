#pragma once

#include <atomic>
#include <cstdint>
#include <iterator>

namespace mvi {

struct BipBufferSPMCIterator {
  using iterator_category = std::forward_iterator_tag;
  using value_type = std::atomic<uint64_t>;
  using difference_type = std::ptrdiff_t;
  using pointer = std::atomic<uint64_t>*;
  using reference = std::atomic<uint64_t>&;

  pointer ptr;

  BipBufferSPMCIterator(pointer p) : ptr(p) {}

  reference operator*() const { return *ptr; }

  pointer operator->() { return ptr; }

  // Prefix increment
  BipBufferSPMCIterator& operator++() {
    ptr++;
    return *this;
  }

  // Postfix increment
  BipBufferSPMCIterator operator++(int) {
    BipBufferSPMCIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  friend bool operator==(const BipBufferSPMCIterator& a, const BipBufferSPMCIterator& b) {
    return a.ptr == b.ptr;
  }

  friend bool operator!=(const BipBufferSPMCIterator& a, const BipBufferSPMCIterator& b) {
    return a.ptr != b.ptr;
  }
};

} // namespace mvi
