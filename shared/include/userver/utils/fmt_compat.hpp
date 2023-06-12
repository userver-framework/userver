#pragma once

#include <fmt/core.h>

#if FMT_VERSION < 80000
#define USERVER_FMT_CONST

namespace fmt {

template <typename S>
const S& runtime(const S& s) {
  return s;
}

}  // namespace fmt
#else
#define USERVER_FMT_CONST const
#endif
