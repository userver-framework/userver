#pragma once

/// @file userver/compiler/impl/asan.hpp
/// @brief Defines USERVER_IMPL_HAS_ASAN to 1
/// if Address Sanitizer is enabled; otherwise
/// defines USERVER_IMPL_HAS_ASAN to 0.

#if defined(__has_feature)

#if __has_feature(address_sanitizer)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_HAS_ASAN 1
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_HAS_ASAN 0
#endif

#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_HAS_ASAN 0
#endif
