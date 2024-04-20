#pragma once

/// @file userver/compiler/impl/tsan.hpp
/// @brief Defines USERVER_IMPL_HAS_TSAN to 1 and includes
/// <sanitizer/tsan_interface.h> if Thread Sanitizer is enabled; otherwise
/// defines USERVER_IMPL_HAS_TSAN to 0.

#if defined(__has_feature)

#if __has_feature(thread_sanitizer)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_HAS_TSAN 1
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_HAS_TSAN 0
#endif

#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_HAS_TSAN 0
#endif

#if USERVER_IMPL_HAS_TSAN
#include <sanitizer/tsan_interface.h>
#endif

#if USERVER_IMPL_HAS_TSAN && !defined(BOOST_USE_TSAN)
#error Broken CMake: thread sanitizer is used however Boost is unaware of it
#endif

#if !USERVER_IMPL_HAS_TSAN && defined(BOOST_USE_TSAN)
#error Broken CMake: thread sanitizer is not used however Boost thinks it is
#endif

#ifdef __clang__
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_DISABLE_TSAN __attribute__((no_sanitize_thread))
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_DISABLE_TSAN
#endif
