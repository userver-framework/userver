#pragma once

#include <cstdint>

USERVER_NAMESPACE_BEGIN

namespace cache::impl::utils {
template <typename T>
struct Jenkins {
  uint32_t operator()(const T&);
};
}  // namespace cache::impl::utils

USERVER_NAMESPACE_END