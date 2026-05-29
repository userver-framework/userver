#pragma once

/// @file userver/utils/fmt_compat.hpp
/// @brief Compatibility shims for different {fmt} library versions.
/// @ingroup userver_universal

#include <fmt/core.h>

#if FMT_VERSION < 80000
#define USERVER_FMT_CONST

/// @brief `{fmt}` compatibility shims across supported versions.
namespace fmt {

template <typename S>
const S& runtime(const S& s) {
    return s;
}

}  // namespace fmt
#else
#define USERVER_FMT_CONST const
#endif
