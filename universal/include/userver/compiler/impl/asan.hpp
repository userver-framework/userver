#pragma once

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
