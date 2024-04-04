#pragma once

#if defined(__has_feature)

#if __has_feature(leak_sanitizer) || __has_feature(address_sanitizer)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_HAS_LSAN 1
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_HAS_LSAN 0
#endif

#else

#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_LEAK__)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_HAS_LSAN 1
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define USERVER_IMPL_HAS_LSAN 0
#endif

#endif

#if USERVER_IMPL_HAS_LSAN
#include <sanitizer/lsan_interface.h>
#endif

USERVER_NAMESPACE_BEGIN

namespace compiler::impl {

template <typename T>
void LsanIgnoreObject([[maybe_unused]] const T* object) {
#if USERVER_IMPL_HAS_LSAN
  __lsan_ignore_object(object);
#endif
}

}  // namespace compiler::impl

USERVER_NAMESPACE_END
