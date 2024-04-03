#pragma once

#if defined(__x86_64__) && defined(__linux__) && \
    !defined(USERVER_DISABLE_RSEQ_ACCELERATION)
#define USERVER_IMPL_HAS_RSEQ
#endif

#ifdef USERVER_IMPL_HAS_RSEQ

#include <cstddef>
#include <cstdint>
#include <thread>

#include <rseq/rseq.h>

#include <userver/compiler/impl/constexpr.hpp>
#include <userver/utils/assert.hpp>
#include <userver/utils/impl/static_registration.hpp>

USERVER_NAMESPACE_BEGIN

namespace concurrent::impl {

inline constexpr std::size_t kRseqArraySizeUninitialized = -1;
inline constexpr std::size_t kRseqArraySizeDisabled = 0;

inline USERVER_IMPL_CONSTINIT std::size_t rseq_array_size =
    kRseqArraySizeUninitialized;

__attribute__((constructor)) inline std::size_t GetRseqArraySize() noexcept {
  if (rseq_array_size == kRseqArraySizeUninitialized) {
    utils::impl::AssertStaticRegistrationAllowed("GetRseqArraySize");

    rseq_register_current_thread();

    rseq_array_size = rseq_available(RSEQ_AVAILABLE_QUERY_LIBC)
                          // libc owns rseq. It automatically registers rseq at
                          // that start of each thread.
                          ? std::thread::hardware_concurrency()

                          // Either rseq is unavailable, or it is unsupported by
                          // libc, in which case rseq initialization can be
                          // thread-unsafe in the presence of coroutines.
                          : kRseqArraySizeDisabled;
  }

  return rseq_array_size;
}

__attribute__((const)) inline std::size_t GetRseqArraySizeUnsafe() noexcept {
  UASSERT(rseq_array_size != kRseqArraySizeUninitialized);
  return rseq_array_size;
}

inline bool IsCpuIdValid(std::uint32_t cpu_id) noexcept {
  if (rseq_unlikely(cpu_id >= GetRseqArraySizeUnsafe())) {
    UASSERT_MSG(
        GetRseqArraySizeUnsafe() == kRseqArraySizeDisabled,
        "CPU count is not properly detected. Please report this issue. "
        "You may want to set USERVER_DISABLE_RSEQ_ACCELERATION=ON cmake "
        "variable to overcome this error.");
    return false;
  }
  return true;
}

}  // namespace concurrent::impl

USERVER_NAMESPACE_END

#endif
