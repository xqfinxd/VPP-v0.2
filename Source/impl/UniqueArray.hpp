#pragma once

#include <memory>

namespace VPP {
namespace impl {
template <class Elem_>
using uarray = std::unique_ptr<Elem_[]>;

template <class Elem_>
inline uarray<Elem_> NewArray(size_t size_) noexcept {
  return std::make_unique<Elem_[]>(size_);
}
}  // namespace impl
}  // namespace VPP